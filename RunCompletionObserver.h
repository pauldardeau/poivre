// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_RUNCOMPLETIONOBSERVER_H
#define POIVRE_RUNCOMPLETIONOBSERVER_H



namespace poivre
{
   class Runnable;

/**
 * RunCompletionObserver is an interface used for callbacks to be notified
 * when a Runnable has completed its run.
 */
class RunCompletionObserver
{
public:
   /**
    * Destructor
    */
   virtual ~RunCompletionObserver() {}
   
   /**
    * Notifies the observer that the Runnable has completed
    * @param runnable the Runnable that has completed
    */
   virtual void notifyRunComplete(Runnable* runnable) = 0;
};

}

#endif
