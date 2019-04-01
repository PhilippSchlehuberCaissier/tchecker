/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_STATEMENT_HH
#define TCHECKER_STATEMENT_HH

#include <iostream>
#include <string>

#include <boost/core/noncopyable.hpp>

#include "tchecker/expression/expression.hh"

/*!
 \file statement.hh
 \brief Syntax tree for statements
 */

namespace tchecker {
  
  class statement_visitor_t;  // forward declaration
  
  
  
  
  /*!
   \class statement_t
   \brief Base class for statements
   */
  class statement_t : private boost::noncopyable {
  public:
    /*!
     \brief Constructor
     */
    statement_t() = default;
    
    /*!
     \brief Destructor
     */
    virtual ~statement_t() = default;
    
    /*!
     \brief Assignment operator (DELETED)
     */
    tchecker::statement_t & operator= (tchecker::statement_t const &) = delete;
    
    /*!
     \brief Move assignment operator (DELETED)
     */
    tchecker::statement_t & operator= (tchecker::statement_t &&) = delete;
    
    /*!
     \brief Clone
     \return A clone of this
     */
    tchecker::statement_t * clone() const;
    
    /*!
     \brief Visit
     \param v : visitor
     */
    void visit(tchecker::statement_visitor_t & v) const;
    
    /*!
     \brief Accessor
     \return this statement as a string
     */
    std::string to_string() const;
  protected:
    /*!
     \brief Output the statement
     \param os : output stream
     */
    virtual std::ostream & do_output(std::ostream & os) const = 0;
    
    /*!
     \brief Clone
     */
    virtual tchecker::statement_t * do_clone() const = 0;
    
    /*!
     \brief Visit
     \param v : visitor
     */
    virtual void do_visit(tchecker::statement_visitor_t & v) const =0;
    
    friend std::ostream & operator<< (std::ostream & os, tchecker::statement_t const & stmt);
  };
  
  
  
  /*!
   \brief Output operator for statements
   \param os : output stream
   \param stmt : statement
   \post stmt has been output to os
   \return os after stmt has been output
   */
  std::ostream & operator<< (std::ostream & os, tchecker::statement_t const & stmt);
  
  
  
  
  /*!
   \class nop_statement_t
   \brief No instruction
   */
  class nop_statement_t : public virtual tchecker::statement_t {
  public:
    /*!
     \brief Constructor
     */
    nop_statement_t();
    
    /*!
     \brief Destructor
     */
    virtual ~nop_statement_t();
  protected:
    /*!
     \brief Output the statement
     \param os : output stream
     \post this has been output to os
     \return os after this has been output
     */
    virtual std::ostream & do_output(std::ostream & os) const;
    
    /*!
     \brief Clone
     \return A clone of this
     */
    virtual tchecker::statement_t * do_clone() const;
    
    /*!
     \brief Visit
     \param v : visitor
     \post v.visit(*this) has been called
     */
    virtual void do_visit(tchecker::statement_visitor_t & v) const;
  };
  
  
  
  
  /*!
   \class assign_statement_t
   \brief assignment
   */
  class assign_statement_t : public virtual tchecker::statement_t {
  public:
    /*!
     \brief Constructor
     \param lvalue : left-value expression
     \param rvalue : right-value expression
     \pre lvalue != nullptr and rvalue != nullptr
     \throw std::invalid_argument : if one of lvalue and rvalue is nullptr
     \note this takes ownerhsip on lvalue and rvalue
     */
    assign_statement_t(tchecker::lvalue_expression_t const * lvalue, tchecker::expression_t const * rvalue);
    
    /*!
     \brief Destructor
     */
    virtual ~assign_statement_t();
    
    /*!
     \brief Accessor
     \return Left value
     */
    inline tchecker::lvalue_expression_t const & lvalue() const
    {
      return (* _lvalue);
    }
    
    /*!
     \brief Accessor
     \return Right value
     */
    inline tchecker::expression_t const & rvalue() const
    {
      return (* _rvalue);
    }
  protected:
    /*!
     \brief Output the statement
     \param os : output stream
     \post this has been output to os
     \return os after this has been output
     */
    virtual std::ostream & do_output(std::ostream & os) const;
    
    /*!
     \brief Clone
     \return A clone of this
     */
    virtual tchecker::statement_t * do_clone() const;
    
    /*!
     \brief Visit
     \param v : visitor
     \post v.visit(*this) has been called
     */
    virtual void do_visit(tchecker::statement_visitor_t & v) const;
    
    tchecker::lvalue_expression_t const * _lvalue;   /*!< Left value */
    tchecker::expression_t const * _rvalue;          /*!< Right value*/
  };
  
  
  
  
  /*!
   \class sequence_statement_t
   \brief Sequential composition of statements
   */
  class sequence_statement_t : public virtual tchecker::statement_t {
  public:
    /*!
     \brief Constructor
     \param first : first statement
     \param second : second statement
     \pre first != nullptr and second != nullptr
     \throw std::invalid_argument : if one of first and second is nullptr
     \note this takes ownership on first and second
     */
    sequence_statement_t(tchecker::statement_t const * first, tchecker::statement_t const * second);
    
    /*!
     \brief Destructor
     */
    virtual ~sequence_statement_t();
    
    /*!
     \brief Accessor
     \return First statement
     */
    inline tchecker::statement_t const & first() const
    {
      return (* _first);
    }
    
    /*!
     \brief Accessor
     \return Second statement
     */
    inline tchecker::statement_t const & second() const
    {
      return (* _second);
    }
  protected:
    /*!
     \brief Output the statement
     \param os : output stream
     \post this has been output to os
     \return os after this has been output
     */
    virtual std::ostream & do_output(std::ostream & os) const;
    
    /*!
     \brief Clone
     \return A clone of this
     */
    virtual tchecker::statement_t * do_clone() const;
    
    /*!
     \brief Visit
     \param v : visitor
     \post v.visit(*this) has been called
     */
    virtual void do_visit(tchecker::statement_visitor_t & v) const;
    
    tchecker::statement_t const * _first;   /*!< First statement */
    tchecker::statement_t const * _second;  /*!< Second statement */
  };
  
  
  
  
  /*!
   \class statement_visitor_t
   \brief Visitor for statements
   */
  class statement_visitor_t {
  public:
    /*!
     \brief Constructor
     */
    statement_visitor_t() = default;
    
    /*!
     \brief Copy constructor
     */
    statement_visitor_t(tchecker::statement_visitor_t const &) = default;
    
    /*!
     \brief Move constructor
     */
    statement_visitor_t(tchecker::statement_visitor_t &&) = default;
    
    /*!
     \brief Destructor
     */
    virtual ~statement_visitor_t() = default;
    
    /*!
     \brief Assignment oeprator
     */
    tchecker::statement_visitor_t & operator= (tchecker::statement_visitor_t const &) = default;
    
    /*!
     \brief Move assignment oeprator
     */
    tchecker::statement_visitor_t & operator= (tchecker::statement_visitor_t &&) = default;
    
    /*!
     \brief Visitors
     */
    virtual void visit(tchecker::nop_statement_t const &) = 0;
    virtual void visit(tchecker::assign_statement_t const &) = 0;
    virtual void visit(tchecker::sequence_statement_t const &) = 0;
  };
  
  
} // end namespace tchecker

#endif // TCHECKER_STATEMENT_HH
