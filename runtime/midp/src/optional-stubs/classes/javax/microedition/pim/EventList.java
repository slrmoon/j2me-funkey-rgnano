package javax.microedition.pim;

import java.util.Enumeration;

public interface EventList extends PIMList {
    int STARTING = 0;
    int ENDING = 1;
    int OCCURRING = 2;

    Event createEvent();

    Event importEvent(Event item);

    void removeEvent(Event item) throws PIMException;

    Enumeration items(int searchType, long startDate, long endDate, boolean initialEventOnly)
            throws PIMException;

    int[] getSupportedRepeatRuleFields(int frequency);
}
