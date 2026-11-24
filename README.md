# COMP16321 Week 7 Lab &mdash; Win32 Implementation
A native Win32 implementation of the application from the COMP16321 Week 7 Lab at the University of Manchester.
<br><br>
The original lab application was implemented using Python and Tkinter. This project recreates the application in C using the Win32 API, while introducing additional features and improvements beyond the original implementation.

## Additional Features and Improvements
### Original Functionality
- Add tasks using a text input field and an Add Task button.
- Add tasks by pressing Enter.
- Toggle task completion by clicking a task.
- Completed tasks are displayed in grey with strikeout text.

### Improvements
- Scrollable task list when tasks exceed the visible area.

### Additional Features
- Save and Save As support using a custom `.tdl` file format.
- Open previously saved task lists.
- Custom names for task lists, independent for their filenames.
- Always on Top mode.
- Font customisation.
- Task deletion.
- Task renaming.
