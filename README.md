# SmartFarm
Final Project

1.Pthread
 -Pthread(POSIX thread)
 It is often called Pthread and provides a set of C standard libraries to support threads with a collection of thread functions proposed by POSIX as standard.
 
- Functions related to pthread
  pthread_create : Create pthread.
  Pthread_join : Wait for the specific pthread to terminate, then release the resource when the specific pthread exits. 
  pthread_exit : Used to terminate the currently running thread.
  
2. Smart Farm
  - Project using raspberry pie and various sensors

  - Requirements
    1. Measure temperature and luminosity every second. 
    2. The fan operates when the temperature exceeds a certain temperature. 
    3. The LED will operate when it is darker than the set brightness. 
    4. Measured values ​​are stored in the database at regular intervals.
    5. Functions use threads because they must operate independently. 

   - A buffer for identifying access to a critical region

   - Buffer is a shared resource

   - To protect shared resources, a mechanism called mutex is used in a multithreaded environment.

   - The data type of the mutex variable is pthread_mutex_t.

   - A mutex locks a shared resource before it is accessed and uses a shared resource to release the lock when it is complete

 
