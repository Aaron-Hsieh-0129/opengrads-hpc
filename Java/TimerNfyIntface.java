/**
  This interface defined the method(s) to be called by the Timer class
  to perform specified action when timer expired.
  This methods are to be implemented by caller (client) of Timer class.

 @author Pedro T.H. Tsai
 @version 1.0
 @Date 2009 Jan 25
*/

import java.util.*;

public interface TimerNfyIntface
{
   /**
    Prototype of the timer callback method.
    @param timer    The timer object that invoke the callback method.
    @param pv_obj   Reference to an data object specified by the caller.
   */
   void action(Timer timer, Object pv_obj);

}









