/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_FSM_DETAILS_MODEL_HH
#define TCHECKER_FSM_DETAILS_MODEL_HH

#include <vector>

#include "tchecker/expression/typechecking.hh"
#include "tchecker/expression/typed_expression.hh"
#include "tchecker/flat_system/model.hh"
#include "tchecker/fsm/details/static_analysis.hh"
#include "tchecker/statement/typechecking.hh"
#include "tchecker/statement/typed_statement.hh"
#include "tchecker/utils/iterator.hh"
#include "tchecker/utils/log.hh"
#include "tchecker/vm/compilers.hh"
#include "tchecker/vm/vm.hh"

/*!
 \file model.hh
 \brief Finite state machine model (details)
 */

namespace tchecker {
  
  namespace fsm {
    
    namespace details {
      
      /*!
       \class model_t
       \brief Model for finite-state machines: model for flat system + bytecode
       for location invariants and for edges guards and statements
       \tparam SYSTEM : type of system, should inherit from tchecker::fsm::details::system_t
       \tparam VM_VARIABLES : type of system variables accessor, should have same signature as
       tchecker::fsm::details::vm_variables_t
       \note Instances cannot be constructed. Refer to class
       tchecker::flat_system::details::model_t for the reason why.
       */
      template <class SYSTEM, class VM_VARIABLES>
      class model_t : public tchecker::flat_system::model_t<SYSTEM> {
      public:
        /*!
         \brief Copy constructor
         \param model : a model
         \post this is a copy of model
         */
        model_t(tchecker::fsm::details::model_t<SYSTEM, VM_VARIABLES> const & model)
        : tchecker::flat_system::model_t<SYSTEM>(model)
        {
          tchecker::log_t log;  // no output
          compile(*this->_system, log);
          assert(log.error_count() == 0);
        }
        
        /*!
         \brief Move constructor
         */
        model_t(tchecker::fsm::details::model_t<SYSTEM, VM_VARIABLES> &&) = default;
        
        /*!
         \brief Destructor
         */
        ~model_t()
        {
          free_memory();
        }
        
        /*!
         \brief Assignment operator
         \param model : a model
         \param this is a copy of model
         */
        tchecker::fsm::details::model_t<SYSTEM, VM_VARIABLES> &
        operator= (tchecker::fsm::details::model_t<SYSTEM, VM_VARIABLES> const & model)
        {
          if (this != &model) {
            free_memory();
            
            tchecker::flat_system::model_t<SYSTEM>::operator=(model);
            
            _vm_variables = model._vm_variables;
            tchecker::log_t log;  // no output
            compile(*this->_system, log);
            assert(log.error_count() == 0);
          }
          return *this;
        }
        
        /*!
         \brief Move-assignment operator
         */
        tchecker::fsm::details::model_t<SYSTEM, VM_VARIABLES> &
        operator= (tchecker::fsm::details::model_t<SYSTEM, VM_VARIABLES> && model) = default;
        
        /*!
         \brief Accessor
         \param loc_id : location ID
         \pre loc_id is less than the number of locations in the model (checked
         by assertion)
         \return type-checked invariant of location loc_id
         */
        tchecker::typed_expression_t const & typed_invariant(tchecker::loc_id_t loc_id) const
        {
          assert(loc_id < _invariants_bytecode.size());
          return * _typed_invariants[loc_id];
        }
        
        /*!
         \brief Accessor
         \param edge_id : edge ID
         \pre edge_id is less than the number of edges in the model (checked
         by assertion)
         \return type-checked guard of edge edge_id
         */
        tchecker::typed_expression_t const & typed_guard(tchecker::edge_id_t edge_id) const
        {
          assert(edge_id < _guards_bytecode.size());
          return * _typed_guards[edge_id];
        }
        
        /*!
         \brief Accessor
         \param edge_id : edge ID
         \pre edge_id is less than the number of edges in the model (checked
         by assertion)
         \return type-checked statement of edge edge_id
         */
        tchecker::typed_statement_t const & typed_statement(tchecker::edge_id_t edge_id) const
        {
          assert(edge_id < _statements_bytecode.size());
          return * _typed_statements[edge_id];
        }
        
        /*!
         \brief Accessor
         \param loc_id : location ID
         \pre loc_id is less than the number of locations in the model (checked
         by assertion)
         \return pointer to first instruction of bytecode for the invariant of location loc_id
         */
        tchecker::bytecode_t const * invariant_bytecode(tchecker::loc_id_t loc_id) const
        {
          assert(loc_id < _invariants_bytecode.size());
          return _invariants_bytecode[loc_id];
        }
        
        /*!
         \brief Accessor
         \param edge_id : edge ID
         \pre edge_id is less than the number of edges in the model (checked
         by assertion)
         \return pointer to first instruction of bytecode for the guard of edge edge_id
         */
        tchecker::bytecode_t const * guard_bytecode(tchecker::edge_id_t edge_id) const
        {
          assert(edge_id < _guards_bytecode.size());
          return _guards_bytecode[edge_id];
        }
        
        /*!
         \brief Accessor
         \param edge_id : edge ID
         \pre edge_id is less than the number of edges in the model (checked
         by assertion)
         \return pointer to first instruction of bytecode for the statement of edge edge_id
         */
        tchecker::bytecode_t const * statement_bytecode(tchecker::edge_id_t edge_id) const
        {
          assert(edge_id < _statements_bytecode.size());
          return _statements_bytecode[edge_id];
        }
        
        /*!
         \brief Accessor
         \return System variables accessor for VM
         */
        VM_VARIABLES const & vm_variables() const
        {
          return _vm_variables;
        }
      protected:
        /*!
         \brief Constructor
         \param system : a system
         \param log : logging facility
         \post this consists in system + synchronizer + bytecode for system
         locations and edges
         \throw std::runtime_error : if system has a weakly synchronized event with a non-trivial guard
         \throw std::runtime_error : if guards, statements, invariants in system cannot be compiled into bytecode
         */
        model_t(SYSTEM * system, tchecker::log_t & log)
        : tchecker::flat_system::model_t<SYSTEM>(system)
        {
          if (tchecker::fsm::details::has_guarded_weakly_synchronized_event(*system))
            throw std::runtime_error("Weakly synchronized event shall not be guarded");

          compile(*this->_system, log);
                                     
          if (log.error_count() > 0)
            throw std::runtime_error("System compilation failure");
        }
        
        /*!
         \brief Compile system bytecode
         \param system : a system
         \param log : logging facility
         \post system's bytcode has been generated. All warnings and errors have
         been reported to log
         */
        void compile(SYSTEM const & system, tchecker::log_t & log)
        {
          compile_invariants(system, log);
          compile_guards(system, log);
          compile_statements(system, log);
        }
        
        /*!
         \brief Compile invariants bytecode
         \param system : a system
         \param log : logging facility
         \post system's typed invariants and invariants bytecode has been
         generated in _typed_invariants and _invariants_bytecode respectively.
         All warnings and errors have been reported to log
         */
        void compile_invariants(SYSTEM const & system, tchecker::log_t & log)
        {
          if ( ! _typed_invariants.empty() )
            throw std::runtime_error("typed invariants not empty");
          if ( ! _invariants_bytecode.empty() )
            throw std::runtime_error("invariants bytecode not empty");
          
          _typed_invariants.resize(system.locations_count(), nullptr);
          _invariants_bytecode.resize(system.locations_count(), nullptr);
          
          tchecker::range_t<typename SYSTEM::const_loc_iterator_t> locations = system.locations();
          for (typename SYSTEM::loc_t const * loc : locations) {
            _typed_invariants[loc->id()] = typecheck(loc->invariant(), system, log,
                                                     "Attribute invariant: " + loc->invariant().to_string());
            try {
              _invariants_bytecode[loc->id()] = tchecker::compile(*_typed_invariants[loc->id()]);
            }
            catch (std::exception const & e)
            {
              log.error("Attribute invariant: " + loc->invariant().to_string(), e.what());
            }
          }
        }
        
        /*!
         \brief Compile guards bytecode
         \param system : a system
         \param log : logging facility
         \post system's typed guards and guards bytecode has been
         generated in _typed_guards and _guards_bytecode respectively.
         All warnings and errors have been reported to log
         */
        void compile_guards(SYSTEM const & system, tchecker::log_t & log)
        {
          if ( ! _typed_guards.empty() )
            throw std::runtime_error("typed guards not empty");
          if ( ! _guards_bytecode.empty() )
            throw std::runtime_error("guards bytecode not empty");
          
          _guards_bytecode.resize(system.edges_count());
          _typed_guards.resize(system.edges_count());
          
          tchecker::range_t<typename SYSTEM::const_edge_iterator_t> edges = system.edges();
          for (typename SYSTEM::edge_t const * edge : edges) {
            _typed_guards[edge->id()] = typecheck(edge->guard(), system, log, "Attribute provided: " + edge->guard().to_string());
            try {
              _guards_bytecode[edge->id()] = tchecker::compile(*_typed_guards[edge->id()]);
            }
            catch (std::exception const & e)
            {
              log.error("Attribute provided: " + edge->guard().to_string(), e.what());
            }
          }
        }
        
        /*!
         \brief Compile statements bytecode
         \param system : a system
         \param log : logging facility
         \post system's typed statements and statements bytecode has been
         generated in _typed_statements and _statements_bytecode
         respectively. All warnings and errors have been reported to log
         */
        void compile_statements(SYSTEM const & system, tchecker::log_t & log)
        {
          if ( ! _typed_statements.empty() )
            throw std::runtime_error("typed statements not empty");
          if ( ! _statements_bytecode.empty() )
            throw std::runtime_error("statements bytecode not empty");
          
          _statements_bytecode.resize(system.edges_count());
          _typed_statements.resize(system.edges_count());
          
          tchecker::range_t<typename SYSTEM::const_edge_iterator_t> edges = system.edges();
          for (typename SYSTEM::edge_t const * edge : edges) {
            _typed_statements[edge->id()] = typecheck(edge->statement(), system, log,
                                                      "Attribute do: " + edge->statement().to_string());
            try {
              _statements_bytecode[edge->id()] = tchecker::compile(*_typed_statements[edge->id()]);
            }
            catch (std::exception const & e)
            {
              log.error("Attribute do: " + edge->statement().to_string(), e.what());
            }
          }
        }
        
        /*!
         \brief Typecheck an expression
         \param expr : expression
         \param system : a system
         \param log : logging facility
         \param context_msg : contextual message for logging
         \return Typed expression for expr in system. All errors and warnings have been
         reported to log
         */
        tchecker::typed_expression_t * typecheck(tchecker::expression_t const & expr,
                                                 SYSTEM const & system,
                                                 tchecker::log_t & log,
                                                 std::string const & context_msg)
        {
          return tchecker::typecheck(expr, _vm_variables.intvars(system), _vm_variables.clocks(system),
                                     [&] (std::string const & msg) { log.error(context_msg, msg); });
        }
        
        /*!
         \brief Typecheck s statement
         \param stmt : statement
         \param system : a system
         \param log : logging facility
         \param context_msg : contextual message for logging
         \return Typed statement for stmt in system. All errors and warnings have been
         reported to log
         */
        tchecker::typed_statement_t * typecheck(tchecker::statement_t const & stmt,
                                                SYSTEM const & system,
                                                tchecker::log_t & log,
                                                std::string const & context_msg)
        {
          return tchecker::typecheck(stmt, _vm_variables.intvars(system), _vm_variables.clocks(system),
                                     [&] (std::string const & msg) { log.error(context_msg, msg); });
        }
        
        /*!
         \brief Free memory
         */
        void free_memory()
        {
          for (tchecker::typed_expression_t * e : _typed_invariants)
            delete e;
          _typed_invariants.clear();
          for (tchecker::typed_expression_t * e : _typed_guards)
            delete e;
          _typed_guards.clear();
          for (tchecker::typed_statement_t * s : _typed_statements)
            delete s;
          _typed_statements.clear();
          for (tchecker::bytecode_t * b : _invariants_bytecode)
            delete[] b;
          _invariants_bytecode.clear();
          for (tchecker::bytecode_t * b : _guards_bytecode)
            delete[] b;
          _guards_bytecode.clear();
          for (tchecker::bytecode_t * b : _statements_bytecode)
            delete[] b;
          _statements_bytecode.clear();
        }
        
        VM_VARIABLES _vm_variables;                                     /*!< System variables accessor for VM */
        std::vector<tchecker::typed_expression_t *> _typed_invariants;  /*!< Type-checked locations invariants */
        std::vector<tchecker::typed_expression_t *> _typed_guards;      /*!< Type-checked edges guards */
        std::vector<tchecker::typed_statement_t *> _typed_statements;   /*!< Type-checked edges statements */
        std::vector<tchecker::bytecode_t *> _invariants_bytecode;       /*!< Bytecode for locations invariants */
        std::vector<tchecker::bytecode_t *> _guards_bytecode;           /*!< Bytecode for edges guards */
        std::vector<tchecker::bytecode_t *> _statements_bytecode;       /*!< Bytecode for edges statements */
      };
      
    } // end of namespace details
    
  } // end of namespace fsm
  
} // end of namespace tchecker

#endif // TCHECKER_FSM_HH

