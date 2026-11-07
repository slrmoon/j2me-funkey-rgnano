package javax.microedition.pim;

import java.util.Enumeration;

public interface ToDoList extends PIMList {
    ToDo createToDo();

    ToDo importToDo(ToDo item);

    void removeToDo(ToDo item) throws PIMException;

    Enumeration items(int field, long startDate, long endDate) throws PIMException;
}
