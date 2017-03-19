// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_AUTOPOINTER_H
#define POIVRE_AUTOPOINTER_H


namespace poivre
{

/**
 * AutoPointer exists to do RAII (resource acquisition is initialization).
 * Old c++ had an auto_ptr that's been deprecated, and new c++ has
 * std::unique_ptr and std::shared_ptr. We've rolled our own so that we can
 * work with current (c++11) as well as old c++ compilers.
 */
template <class T>
class AutoPointer
{
public:
   /**
    * Constructs an AutoPointer that contains (owns) an object instance
    * @param objectPointer the pointer to the object that will now be owned
    */
   AutoPointer(T objectPointer) :
      m_object(objectPointer) {
   }

   /**
    * Destructor - frees the owned object
    */
   ~AutoPointer() {
      delete m_object;
   }

   /**
    * Arrow operator - returns the owned object pointer
    * @return pointer to owned object
    */
   T& operator->() { return m_object; }
   
   T& operator()() { return m_object; }

   
   /**
    * Assigns new object to be owned by the AutoPointer. Deletes old one if present.
    * @param copyObject the new object that will be owned.
    */
   void assign(const T& copyObject) {
      if (m_object != copyObject) {
         if (m_object) {
            delete m_object;
         }
      
         m_object = copyObject;
      }
   }

   /**
    * Determines if the AutoPointer has an object or not
    * @return true if AutoPointer has an object, false otherwise
    */
   bool haveObject() const {
      return (NULL != m_object);
   }

   T m_object;   

private:
   
   // no copies
   AutoPointer(const AutoPointer<T>&);
   AutoPointer<T>& operator=(const AutoPointer<T>&);
};

}

#endif
