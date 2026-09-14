#define WIDTHLINES 100000
#define NUMLINES 10000
#define NUMMENUITEMS 1000
#define HASH_SIZE 8191

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

int* pref_idx[NUMLINES]; // pointer to the indixes of the dishes on which the employee expresses a preference
int8_t* pref_val[NUMLINES]; // pointer to the employee's preference
int num_prefs[NUMLINES]; // number of dishes actually requested by each employee
int is_taut[NUMLINES]; // quick flag for tautology

typedef struct { 
    const char* name; // pointer to the dish name
    int index; // corresponding index of the dish in the matrix columns
} HashSlot;

HashSlot hash_table[HASH_SIZE];

int explore_vars[NUMMENUITEMS]; // in order, contains the indices of the dishes to explore first and later
int global_record = 0; // record of satisfied employees

/*
Calculates and returns the memory address where a specific string will be stored.
For identical strings, the calculated memory address will be the same, giving us a standard pointer.
    const char* s - string whose memory address is to be calculated
*/
unsigned long hash_str(const char* s) {
    unsigned long h = 5381;
    int c;
    while ((c = *s++)) h = ((h << 5) + h) + (unsigned char)c; // the loop stops automatically at c == "\0" // accented letters could be negative, hence unsigned char
    return h;
}

/*
Allocates strings inside hash_table; if two strings happen to have the same calculated memory address,
the function resolves it using linear probing.
    char* menu_items - array of available dishes to allocate
    int num_menu_items - number of dishes to allocate
*/
void hash_build(char* menu_items[], int num_menu_items) {
    for (int i = 0; i < HASH_SIZE; i++) hash_table[i].name = NULL; // table initialization
    for (int i = 0; i < num_menu_items; i++) {
        unsigned long h = hash_str(menu_items[i]) % HASH_SIZE; // calculate the dish index in the hash_table
        while (hash_table[h].name != NULL) h = (h + 1) % HASH_SIZE; // linear probing if the index is already occupied
        hash_table[h].name = menu_items[i];
        hash_table[h].index = i; // saves the corresponding matrix column!
    }
}

/*
Searches for the corresponding word in hash_table and returns the column index corresponding to the int_lines_items matrix.
    const char* name - name of the dish to search for in the hashtable
*/
int hash_lookup(const char* name) {
    unsigned long h = hash_str(name) % HASH_SIZE; // calculate the string index in the hash_table
    while (hash_table[h].name != NULL) { // only if the dish exists in the menu
        if (strcmp(hash_table[h].name, name) == 0) return hash_table[h].index; // verify the correctness of the dish
        h = (h + 1) % HASH_SIZE; // try the immediately following hash_table index
    }
    return -1; // dish not found
}

/*
Splits a string into multiple strings using a delimiter character.
    char* source - pointer to the string to split
    const char* delimiter - delimiter character for all strings
    char* dest - pointer to the destination array of string pointers
*/
int split_into_array(char* source, const char* delimiter, char* dest[]) {
    int count = 0;
    int max_items = 100000;
    char* token = strtok(source, delimiter);

    while (token != NULL && count < max_items) {
        dest[count] = strdup(token);   // allocate and copy
        if (dest[count] == NULL) {
            fprintf(stderr, "Allocazione fallita\n");
            exit(1);  // the whole program throws an error
        }
        count++;
        token = strtok(NULL, delimiter);
    }
    if (strcmp(source, "") == 0) return 0;
    return count;
}

/*
In an array of string pointers, removes an element by shifting all
subsequent elements back.
    char* array[] - array of string pointers to modify
    int* count - pointer to the number of elements in that array
    int index - index of the position to remove
*/
void remove_at_index(char* array[], int* count, int index) {
    if (index < 0 || index >= *count) return; // invalid index
    free(array[index]);
    // shifts all elements after "index" back by one position
    memmove(&array[index], &array[index + 1], (*count - index - 1) * sizeof(char*));
    (*count)--; // update the count of valid elements
}

/*
Removes all duplicates present in an array of char pointers.
    char* array[] - array of string pointers to modify
    int* count - pointer to the number of elements in that array
*/
void remove_duplicates(char* array[], int* count) {
    for (int i = 0; i < *count; i++) {
        // compare each element with all subsequent ones
        for (int j = *count - 1; j > i; j--) {
            if (strcmp(array[i], array[j]) == 0) {
                remove_at_index(array, count, j); // automatically decrease the initial count as well
            }
        }
    }
}

/*
Checks if the array contains both negative and positive elements of the same dish in the same
employee's request.
Returns 1 if there are, and 0 if there are not.
    char* array[] - array of string pointers to modify
    int count - number of elements in that array
*/
int are_there_neg_and_pos(char* array[], int count) {
    for (int i = 0; i < count; i++) {
        // compare each element with all subsequent ones
        for (int j = count - 1; j > i; j--) {
            if ((array[j][0] == '-' && strcmp(array[i], &array[j][1]) == 0) || 
                (array[i][0] == '-' && strcmp(array[j], &array[i][1]) == 0)) {
                return 1;
            }
        }
    }
    return 0;
}

/*
Sorts the array of available dishes based on how many times they are mentioned (positively or negatively) in the employees' requests.
This way, we test the most "problematic" dishes first.
    int num_menu_items - total number of available dishes
    int num_lines - total rows
*/
void build_exploration_order(int num_menu_items, int num_lines) {
    int freqs[NUMMENUITEMS] = {0};

    // count of all the times each dish was mentioned by the employees
    for (int i = 0; i < num_lines; i++) {
        if (is_taut[i]) continue; // tautology found
        for (int k = 0; k < num_prefs[i]; k++) {
            freqs[pref_idx[i][k]]++; // freqs[24] = how many times dish 24 is mentioned by employees
        }
    }

    for (int j = 0; j < num_menu_items; j++) {
        explore_vars[j] = j; // appearance order [0, 1, 2, ...]
    }

    for (int j = 0; j < num_menu_items - 1; j++) {
        for (int k = j + 1; k < num_menu_items; k++) {
            // sorting explore_vars based on freqs
            if (freqs[explore_vars[k]] > freqs[explore_vars[j]]) {
                // if explore_vars has [1, 0, 3, 2] it means the first dish to explore is the second one mentioned,
                // the second dish to explore is the first one mentioned, and the third to explore is the third mentioned, etc...
                int temp = explore_vars[j];
                explore_vars[j] = explore_vars[k];
                explore_vars[k] = temp;
            }
        }
    }
}

/*
Checks whether an employee can be satisfied by a partial menu.
Returns 1 if satisfied or if they could still be satisfied by deciding the combination of
dishes not yet decided in the partial menu, and 0 if it is completely impossible to satisfy them in any case.
    int8_t int_menu_items[] - contains the partial menu translated into integers
    int i - employee's position in the hierarchy (in ascending order)
*/
int is_line_ok(int8_t int_menu_items[], int i) {
    if (is_taut[i]) return 1; // tautology found
    for (int k = 0; k < num_prefs[i]; k++) {
        int dish = pref_idx[i][k];
        int want = pref_val[i][k];
        if (int_menu_items[dish] == want || int_menu_items[dish] == 0) { // if the preference is compatible or not expressed
            return 1;
        }
    }
    return 0;
}

/*
Checks if a menu, partial or not, can (even just possibly) satisfy all employees.
    int int_menu_items[] - contains the partial menu translated into integers
    int num_lines - total number of employee rows
*/
int check(int8_t int_menu_items[], int num_lines) {
    for (int i = global_record; i < num_lines; i++) {
        if (!is_line_ok(int_menu_items, i)) return i; // returns the maximum number of satisfiable employees with that menu at the first unsatisfiable employee
    }
    return num_lines;
}

/*
Scans the employees (up to the current 'global_record') with an input partial menu: 
- If an employee has only one valid option left, it automatically forces it into the menu, triggering any chain reactions.
- If an unsatisfied employee has no options left, it immediately decrees the failure of the partial menu and its derivatives.
Returns 1 if the partial menu is still valid and the deductions were successful, whereas it returns 0 if at least one employee
in the globalrecord range is unsatisfiable.
    int int_menu_items[] - contains the partial menu translated into integers
    int num_lines - total number of employee rows
    int forced_list - set of forced choices in the menu to satisfy as many employees as possible
    int *num_forced - pointer to the number of forced choices
*/
int apply_unit_propagation(int8_t int_menu_items[], int num_lines, int forced_list[], int *num_forced) {
    int changed;
    do {
        changed = 0;
        // if an employee is unsatisfied, or if I haven't beaten my record of satisfied employees yet, I continue
        for (int i = 0; i <= global_record && i < num_lines; i++) {
            if (is_taut[i]) continue; // Skip tautology

            int satisfied = 0; // set to 1 when an employee is unequivocally satisfied
            int unassigned_count = 0; // how many dishes at 0 are left for the employee
            int last_unassigned_idx = -1; 
            int last_unassigned_val = 0;

            for (int k = 0; k < num_prefs[i]; k++) { // one shot per expressed preference
                int dish = pref_idx[i][k];  // go to the pointer at index i, then forward by k steps
                int want = pref_val[i][k];

                if (int_menu_items[dish] == want) { // the employee's request is compatible with the proposed menu
                    satisfied = 1;
                    break; 
                } else if (int_menu_items[dish] == 0) { // the request is still potentially compatible
                    unassigned_count++;
                    last_unassigned_idx = dish; // store the index of the dish wanted by the employee
                    last_unassigned_val = want; // store the employee's will
                }
            }

            if (!satisfied) { // the employee is not even potentially satisfied
                if (unassigned_count == 0) {
                    return 0; // there are no other dishes that can potentially satisfy the employee
                } else if (unassigned_count == 1) { // I still have exactly one other possibility to make the employee satisfied
                    int_menu_items[last_unassigned_idx] = last_unassigned_val;
                    forced_list[(*num_forced)++] = last_unassigned_idx; // force the only choice for the next iteration
                    changed = 1; // restart the loop
                }
            }
        }
    } while (changed);

    return 1; // no unsatisfied employee, we can move forward
}


/*
Through the combination of randomly decided dishes and consequently imposed dishes with the help of the apply_unit_propagation function,
this function ensures that global_record equals the maximum number of satisfiable employees with a complete menu.
The function is recursive.
    int depth - index of the dish we are going to examine in the current iteration
    int8_t int_menu_items[] - contains the partial menu translated into integers
    int num_menu_items - number of available dishes on the menu
    int num_lines - total number of employee rows
*/
void solve_recursive(int depth, int8_t int_menu_items[], int num_menu_items, int num_lines) {
    // Base case: Complete menu
    if (depth == num_menu_items) {
        int score = check(int_menu_items, num_lines);
        if (score > global_record) {
            global_record = score;
        }
        return;
    }

    int dish = explore_vars[depth]; // use explore_vars to get the dishes directly sorted by frequency

    // if apply_unit_propagation has already decided this dish, skip to the next
    if (int_menu_items[dish] != 0) {
        solve_recursive(depth + 1, int_menu_items, num_menu_items, num_lines);
        return;
    }

    int forced_list[NUMMENUITEMS]; // Array to remember who we forced
    int num_forced;

    // first I make the presence of a dish positive
    int_menu_items[dish] = 1;
    num_forced = 0;
    
    // does the propagation leave any employees unsatisfied?
    if (apply_unit_propagation(int_menu_items, num_lines, forced_list, &num_forced)) {
        solve_recursive(depth + 1, int_menu_items, num_menu_items, num_lines);
        if (global_record == num_lines) return; // all employees happy
    }
    
    // undo this dish and ALL those forced by the chain reaction
    for(int k = 0; k < num_forced; k++) int_menu_items[forced_list[k]] = 0;
    int_menu_items[dish] = 0;

    if (global_record == num_lines) return;

    // then I make the presence of a dish negative
    int_menu_items[dish] = -1;
    num_forced = 0;
    
    if (apply_unit_propagation(int_menu_items, num_lines, forced_list, &num_forced)) {
        solve_recursive(depth + 1, int_menu_items, num_menu_items, num_lines);
    }
    
    // backtrack
    for(int k = 0; k < num_forced; k++) int_menu_items[forced_list[k]] = 0;
    int_menu_items[dish] = 0;
}

/*
Checks the partial menu combinations and returns the maximum number of satisfied employees across all complete menus.
It is assumed that the very first value of int_menu_items is [0, ..., 0]
    int8_t int_menu_items[] - contains the partial menu translated into integers
    int num_menu_items - number of available dishes on the menu
    int num_lines - total number of employee rows
*/
int check_all_menu_combinations(int8_t int_menu_items[], int num_menu_items, int num_lines) {
    global_record = 0;
    build_exploration_order(num_menu_items, num_lines);
    // starting menu
    for (int i = 0; i < num_menu_items; i++) {
        int_menu_items[i] = 0;
    }
    solve_recursive(0, int_menu_items, num_menu_items, num_lines);
    return global_record;
}

/*
Directly prints the desired output by printing backwards the number of employees that had to be removed 
to obtain the complete menu that satisfied as many employees as possible.
    int8_t int_menu_items[] - contains the partial menu translated into integers
    int num_menu_items - number of available dishes on the menu
    int original_num_lines - total number of employees, not modified by recursion
*/
void final_output(int8_t int_menu_items[], int num_menu_items, int original_num_lines) {
    int result = check_all_menu_combinations(int_menu_items, num_menu_items, original_num_lines);
    int difference = original_num_lines - result;

    if (difference == 0) {
        printf("OK\n"); 
        return;
    }

    printf("KO\n");
    for (int i = 1; i <= difference; i++) {
        printf("-%d\n", i);
    }
    printf("OK\n");
}

/*
This function is aimed to fulfill is_taut, num_prefs, pref_idx, pref_val with each employee's preferences.
If the menu is [pizza, pasta, sushi] and the employee's preferences are [-pizza, sushi] then
    - pref_idx = [0, 2]
    - pref_val = [-1, 1]
    - num_prefs = 2.
If a tautology is found, such as [pizza, -pizza, sushi], the is_taut flag of the designated employee will be activated.
    char* lines[] - char pointer containing all the lines of the original file
    char* menu_items[] - array of available dishes as strings
    int num_menu_items - total number of available dishes
    int num_lines - total rows
*/
void translate(char* lines[], char* menu_items[], int num_menu_items, int num_lines) {
    int i, num_line_items;
    char* line_items[NUMMENUITEMS * 3];

    hash_build(menu_items, num_menu_items); // build the hash table of the menu dishes

    for (i = 0; i < num_lines; i++) {
        num_line_items = split_into_array(lines[i], " ", line_items);
        remove_duplicates(line_items, &num_line_items);

        if (are_there_neg_and_pos(line_items, num_line_items) == 1) {
            is_taut[i] = 1;
            num_prefs[i] = 0;
            pref_idx[i] = NULL;
            pref_val[i] = NULL;
        } else {
            is_taut[i] = 0;
            pref_idx[i] = malloc(num_line_items * sizeof(int)); // dynamic allocation of all hypothetically necessary space for all dishes
            pref_val[i] = malloc(num_line_items * sizeof(int8_t));
            if ((pref_idx[i] == NULL || pref_val[i] == NULL) && num_line_items > 0) {
                fprintf(stderr, "Allocazione fallita\n");
                exit(1);
            }
            num_prefs[i] = 0;

            for (int k = 0; k < num_line_items; k++) {
                int is_negative = 0;
                if (line_items[k][0] == '-') is_negative = 1;
                char* search_str = is_negative ? &line_items[k][1] : line_items[k]; // if negative, skip '-'
                int idx = hash_lookup(search_str); // finds the value corresponding to the dish in the hash table
                
                if (idx != -1) {
                    pref_idx[i][num_prefs[i]] = idx; // index of the corresponding dish in the hash table
                    pref_val[i][num_prefs[i]] = is_negative ? -1 : 1; // preference value
                    num_prefs[i]++; // employee's number of preferences incremented
                }
            }
        }
        for (int k = 0; k < num_line_items; k++) free(line_items[k]); // free all memory
    }
}

int main() {
    //char input_string[10000];
    char* lines[NUMLINES];
    char temporary_line[WIDTHLINES];
    char* menu[NUMMENUITEMS];
    int num_lines, num_menu_items;
    int i = 0;

    // Read the input using standard STDIN
    while(fgets(temporary_line, WIDTHLINES, stdin) != NULL) { // ends at EOF
        temporary_line[strcspn(temporary_line, "\n")] = '\0'; // replace "\n" with "\0"
        if(strlen(temporary_line) > 0) {
            lines[i] = strdup(temporary_line); // copy the string into lines
            i++;
        }
    }

    // if the file is completely empty, stop everything for safety
    if (i == 0) return 0;
    
    num_menu_items = split_into_array(lines[0], " ", menu); // get all available dishes
    num_lines = i - 1; // save the number of employees

    // Use VLAs to avoid taking up too much memory unnecessarily
    int8_t int_menu_items[num_menu_items];

    // Translate strings into matrices and integers
    translate(&lines[1], menu, num_menu_items, num_lines);
    
    // Final output printing
    final_output(int_menu_items, num_menu_items, num_lines);

    for (int k = 0; k < num_lines; k++) {
        if (!is_taut[k]) {
            free(pref_idx[k]);
            free(pref_val[k]);
        }
    }
    
    // MEMORY CLEANUP
    for(i = num_lines; i >= 0; i--) {
        free(lines[i]);
    }

    for(i = 0; i < num_menu_items; i++) {
        free(menu[i]);
    }

    return 0;
}