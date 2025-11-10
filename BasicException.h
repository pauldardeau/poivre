// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_BASICEXCEPTION_H
#define POIVRE_BASICEXCEPTION_H

#include <string>
#include <exception>


namespace poivre
{

/**
 * The BasicException is a subclass of std::exception that contains
 * the reason for the exception as a std::string.
 */
class BasicException : public std::exception
{
public:
   /**
    * Constructs a BasicException
    * @param what the reason for the exception
    */
   explicit BasicException(const char* what) :
      m_what( what ) {
   }

   /**
    * Constructs a BasicException
    * @param what the reason for the exception
    */
   explicit BasicException(const std::string& what) :
      m_what( what ) {
   }

   /**
    * Copy constructor
    * @param copy the source of the copy
    */
   BasicException(const BasicException& copy) :
      m_what( copy.m_what ) {
   }

   /**
    * Destructor
    */
   virtual ~BasicException() throw () {}

   /**
    * Retrieve the type (class name) of exception
    * @return class name (type) of exception
    */
   virtual const char* getType() const {
      return "BasicException";
   }

   /**
    * Copy operator
    * @param copy the source of the copy
    * @return reference to the updated instance
    */
   BasicException& operator=(const BasicException& copy) {
      if (this == &copy) {
         return *this;
      }

      m_what = copy.m_what;

      return *this;
   }

   /**
    * Retrieves the C string reason for the exception
    * @return C string reason for exception
    */
   virtual const char* what() const throw () {
      return m_what.c_str();
   }

   /**
    * Retrieves the string reason for the exception
    * @return string reason for the exception
    */
   virtual const std::string& whatString() const {
      return m_what;
   }

private:
   std::string m_what;
};

}

#endif

