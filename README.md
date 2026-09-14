# The impossible catering
!THIS IS A SAT-SOLVER PROGRAM!  

The problem is organizing a company dinner. There are different dishes in the menu offered by the restaurant, each identified by a unique name.  
Each employee has a set of preferences, and in order to satisfy an employee at least one of his preferences must be satisfied (such as **I want** pizza **or I don't want** sushi).  
In order for the dinner to be succesful, every emploee must be satisfied. The problem is finding if there is a menu that satisfies each employee, so the company owner decides to order each employee by importance in the company.  

This C program is designed to find a perfect menu for the maximum possible number of top-priority employees, strictly respecting their hierarchical order.

If there isn't a menu that can satisfy everyone, the program will exclude the last employee in importance order and will try to find if there is a possible menu that satisfies the remaining employees.  

## Input format
The input must be formatted strictly.  
The first line is dedicated to define all the possibile dishes offered by the restaurant, each separated by a blankspace (' '), the best possible menu is a combination of those dishes.  
The remaining lines represent all employees' preferences.  
If an employee prefers a dish to be put in the final menu, the dish will be simply mentioned in its line, if an employee doesn't want a dish to be put in the final menu, the dish will be mentioned with a prefixed with a minus sign ("-").

An input example could be:
>   pasta pizza sushi cake  
>   -pasta pizza  
>   pizza sushi -pasta

The first line means that the dishes available are *pasta, pizza, sushi* and *cake*.  
The second line means that the most important employee preferences are *"I don't want the pasta to be in the menu or a I want the pizza to be in the menu"*.
The third line represents as well the preferences of the third employee.  

Expressing a preference more than once is exactly like expressing it once. *pizza pizza pizza* in an employee's line is equivalent to writing *pizza* one time.  
If an employee is happy with a dish being or not being put in the menu at the same time, that employee will always be happy no matter the final menu (tautology).

## Output format
The output will be *"OK"* once all the employees or remaining employees are satisfied.  
If not all employees can be satisfied, the program will be printing *"KO"*, and then it prints the indexes of the employees that had to be sequentially removed to achieve a perfect menu for the remaining group.  

**For example a non satisfiable input is**
>   pizza sushi pasta cake  
>   -pizza sushi -pasta  
>   pizza -sushi pasta  
>   -pasta  
>   -sushi  
>   pasta  
>   sushi  

All the requirements are unsatisfiable for all employees because of the last four ones.
The output of the program will hence be:
>   KO  
>   -1  
>   -2  
>   OK  

**KO** because not all employees can be satisfied;  
**-1** means that we need to cut the last employee preferences;  
**-2** means that we also need to cut the last two employees preferences;  
now we can satisfy all the remaining employees, so we print **OK** 

### Strengths of the program
+ Uses unit propagation to maximize time efficiency.
+ Uses VLAs, and certain data types to minimize memory usage.
+ The code is maintainable and documented, to be easely understandable.

### Requirements and limits of the program
+ Dishes mentioned in the first line must be unique and separated by a blankspace.
+ Dishes mentioned in every employee preferences must be the ones cited in the first line. Unrecognized dishes are ignored by the algorithm and may consequently corrupt the program's output
