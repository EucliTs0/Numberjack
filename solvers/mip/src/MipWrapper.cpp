
#include <iostream>

#include "MipWrapper.hpp"
#include <math.h>

/**************************************************************
 ********************     EXPRESSION        *******************
 **************************************************************/

void MipWrapper_Expression::initialise(bool c){ 
  _lower = _upper = _coef = 0;
  _ident = nbj_ident = -1;
  _expr_encoding = NULL;
  _solver = NULL;
  _continuous = c;
}

MipWrapper_Expression::MipWrapper_Expression(){
  initialise(true);
}

int MipWrapper_Expression::get_size(){
  return _upper - _lower +1;
}

int MipWrapper_Expression::get_max(){
  return _upper;
}

int MipWrapper_Expression::get_min(){
  return _lower;
}

void MipWrapper_Expression::display(){
  std::cout << _coef << "*X" << _ident << " ";
}


MipWrapper_Expression::~MipWrapper_Expression(){}

void MipWrapper_Expression::encode(MipWrapperSolver *solver){
  if(!_expr_encoding) {
    DBG("Creating an encoding with over %d vars\n", get_size());
    
    _expr_encoding = new MipWrapper_Expression*[get_size()];
    _expr_encoding -= (int)_lower;
    LinearConstraint *con = new LinearConstraint(1, 1);
    LinearConstraint *eq_con = new LinearConstraint(0, 0);
    for(int i = _lower; i <= _upper; ++i){
      _expr_encoding[i] = new MipWrapper_IntVar(0, 1);
      _expr_encoding[i]->add(solver, false);
      con->add_coef(_expr_encoding[i], 1);
      eq_con->add_coef(_expr_encoding[i], i);
    }
    eq_con->add_coef(this, -1, false);
    _solver->_constraints.push_back(con);
    _solver->_constraints.push_back(eq_con);
  }
}

bool MipWrapper_Expression::has_been_added() const{
  return _solver != NULL;
}

MipWrapper_Expression* MipWrapper_Expression::add(MipWrapperSolver *solver,
						  bool top_level){
  if(!this->has_been_added()){
    _solver = solver;
    DBG("add variable [%f..%f]\n", _lower, _upper);
    this->_ident = solver->var_counter++;
  }
  return this;
}

/**
 * IntVar stuff
 */

MipWrapper_IntVar::MipWrapper_IntVar(const int lb,
				     const int ub){
  initialise(false);
  _upper = (double)ub;
  _lower = (double)lb;
  _has_holes_in_domain = false;
  DBG("creating integer variable [%d..%d]\n", lb, ub);
}

MipWrapper_IntVar::MipWrapper_IntVar(const int lb,
				     const int ub,
				     const int ident){
  initialise(false);
  _upper = (double)ub;
  _lower = (double)lb;
  _has_holes_in_domain = false;
  nbj_ident = ident;
  DBG("creating integer variable [%d..%d]\n", lb, ub);
}

MipWrapper_IntVar::MipWrapper_IntVar(MipWrapperIntArray& values,
				     const int ident){
  initialise(false);
  
  _values = values;
  _has_holes_in_domain = true;
  nbj_ident = ident;
  
  // Ensure that bounds are set up properly  
  _upper = _values.get_item(0);
  _lower = _values.get_item(0);
  for(int i = 0; i < _values.size(); ++i){
    if(_values.get_item(i) > _upper) _upper = _values.get_item(i);
    if(_values.get_item(i) < _lower) _lower = _values.get_item(i);
  }
  DBG("Creating variable with holes in the domain, size:%d \n", _values.size()); 
}

MipWrapper_Expression* MipWrapper_IntVar::add(MipWrapperSolver* solver,
					      bool top_level){
  MipWrapper_Expression::add(solver, top_level);
  if(_has_holes_in_domain)
    encode(solver);
  return this;
}

void MipWrapper_IntVar::encode(MipWrapperSolver* solver){
  if(!_expr_encoding) {
    if(_has_holes_in_domain){
      DBG("Making an encoding for a variable with holes %s\n", "");

      _expr_encoding = new MipWrapper_Expression*[get_size()];
      for(int i = 0; i < get_size(); ++i) _expr_encoding[i] = NULL;
      _expr_encoding -= (int)_lower;
      
      LinearConstraint *con = new LinearConstraint(1, 1);
      LinearConstraint *eq_con = new LinearConstraint(0, 0);
      for(int i = 0; i < _values.size(); ++i){
	_expr_encoding[_values.get_item(i)] = new MipWrapper_IntVar(0, 1);
	_expr_encoding[_values.get_item(i)]->add(solver, false);
	con->add_coef(_expr_encoding[i], 1);
	eq_con->add_coef(_expr_encoding[i], _values.get_item(i));
      }
      eq_con->add_coef(this, -1);
      
      _solver->_constraints.push_back(con);
      _solver->_constraints.push_back(eq_con);

    } else MipWrapper_Expression::encode(solver);
  }
}

int MipWrapper_IntVar::get_value() const{
  //TODO: Fix me!
  return 0;
}

/**
 * FloatVar stuff
 */

MipWrapper_FloatVar::MipWrapper_FloatVar(){
  initialise(true);
  _upper = 0;
  _lower = 0;
  DBG("Crearing continuous variable (%f .. %f)\n", _upper, _lower);
}

MipWrapper_FloatVar::MipWrapper_FloatVar(const double lb, const double ub){
  initialise(true);
  _upper = ub;
  _lower = lb;
  DBG("Crearing continuous variable (%f .. %f)\n", _upper, _lower);
}

MipWrapper_FloatVar::MipWrapper_FloatVar(const double lb, const double ub, const int ident)
{
  initialise(true);
  _upper = ub;
  _lower = lb;
  nbj_ident = ident;
  DBG("Crearing continuous variable (%f .. %f)\n", _upper, _lower);
}

double MipWrapper_FloatVar::get_value() const{
  //TODO: fix me
  return 0.0;
}

/**
 * Constraint stuff
 */

MipWrapper_add::MipWrapper_add(MipWrapper_Expression *arg1,
			       MipWrapper_Expression *arg2)
  : MipWrapper_Sum(){
  addVar(arg1);
  addVar(arg2);
  addWeight(1);
  addWeight(1);
  initialise();
}

MipWrapper_add::MipWrapper_add(MipWrapper_Expression *arg1,
			       const int arg2)
  : MipWrapper_Sum(){
  addVar(arg1);
  addWeight(1);
  set_rhs(-arg2);
  initialise();
}

MipWrapper_add::~MipWrapper_add(){}

MipWrapper_sub::MipWrapper_sub(MipWrapper_Expression *arg1, const int arg2)
  : MipWrapper_Sum(){
  addVar(arg1);
  addWeight(1);
  set_rhs(arg2);
  initialise();
}

MipWrapper_sub::MipWrapper_sub(MipWrapper_Expression *arg1, MipWrapper_Expression *arg2)
  : MipWrapper_Sum(){
  addVar(arg1);
  addVar(arg2);
  addWeight(1);
  addWeight(-1);
  initialise();
}

MipWrapper_sub::~MipWrapper_sub() {}

MipWrapper_AllDiff::MipWrapper_AllDiff( MipWrapper_Expression* arg1,
				       MipWrapper_Expression* arg2 ) 
  : MipWrapper_Flow(){
  addVar(arg1);
  addVar(arg2);
  initbounds();
  DBG("Creating an AllDiff constraint %s\n", "");
  std::fill(this->card_ub+this->min_val, this->card_ub+this->max_val+1, 1);
}

MipWrapper_AllDiff::MipWrapper_AllDiff( MipWrapperExpArray& vars ) 
  : MipWrapper_Flow(vars) {
  DBG("Creating an AllDiff constraint %s\n", "");
  std::fill(this->card_ub+this->min_val, this->card_ub+this->max_val+1, 1);
}

MipWrapper_AllDiff::~MipWrapper_AllDiff(){}

MipWrapper_Gcc::MipWrapper_Gcc(MipWrapperExpArray& vars,
			       MipWrapperIntArray& vals,
			       MipWrapperIntArray& lb,
			       MipWrapperIntArray& ub ) 
  : MipWrapper_Flow(vars){
  DBG("Creating an GCC constraint %s\n", "");
  int j, n=vals.size();
  for(int i=0; i<n; ++i){
      j = vals.get_item(i);
      this->card_lb[j] = lb.get_item(i);
      this->card_ub[j] = ub.get_item(i);
  }
}

MipWrapper_Gcc::~MipWrapper_Gcc(){}

MipWrapper_Flow::MipWrapper_Flow(MipWrapperExpArray& vars)
  : MipWrapper_FloatVar() {
  _vars = vars;
  initbounds();
}

MipWrapper_Flow::MipWrapper_Flow()
  : MipWrapper_FloatVar() {}

void MipWrapper_Flow::initbounds(){
  int i, lb, ub;
  max_val = -INT_MAX;
  min_val = INT_MAX;
  
  for(i = 0; i < _vars.size(); ++i) {
    lb = (int)(this->_vars.get_item(i)->_lower);
    ub = (int)(this->_vars.get_item(i)->_upper);
    
    if(lb < min_val) min_val = lb;
    if(ub > max_val) max_val = ub;
  }

  card_lb = new int[max_val-min_val+1];
  card_ub = new int[max_val-min_val+1];

  card_lb -= min_val;
  card_ub -= min_val;

  std::fill(card_lb+min_val, card_lb+max_val+1, 0);
  std::fill(card_ub+min_val, card_ub+max_val+1, _vars.size());
}

MipWrapper_Flow::~MipWrapper_Flow(){}

void MipWrapper_Flow::addVar(MipWrapper_Expression* v){ _vars.add(v); }

MipWrapper_Expression* MipWrapper_Flow::add(MipWrapperSolver *solver,
					    bool top_level){
  if(!has_been_added()){

    DBG("Adding in Flow %s\n", "");

    if(top_level){
      
      for(int i = 0; i < _vars.size(); ++i) {
	_vars.get_item(i)->add(solver, false);
	_vars.get_item(i)->encode(solver);
      }
      
      for(int i = min_val; i <= max_val; ++i) {
	LinearConstraint *con = new LinearConstraint(card_lb[i], card_ub[i]);
	for(int j = 0; j < _vars.size(); ++j)
	  if( i <= (int)(_vars.get_item(j)->_upper) &&
	      i >= (int)(_vars.get_item(j)->_lower) &&
	      _vars.get_item(j)->_expr_encoding[i] != NULL)
	      con->add_coef(_vars.get_item(j)->_expr_encoding[i], 1);
	solver->_constraints.push_back(con);     
      }
      
    } else {
      std::cout << "Warning Flow constraint supported only on top level"
	        << std::endl;
      exit(1);
    }
  }
  return this;
}


MipWrapper_Sum::MipWrapper_Sum(MipWrapperExpArray& vars, 
		   MipWrapperIntArray& weights, 
		   const int offset)
  : MipWrapper_FloatVar(){
  _offset = offset;
  _vars = vars;
  _weights = weights;
  initialise();
}

MipWrapper_Sum::MipWrapper_Sum(MipWrapper_Expression *arg1, 
		   MipWrapper_Expression *arg2, 
		   MipWrapperIntArray& w, 
		   const int offset)
  : MipWrapper_FloatVar() {
  _offset = offset;
  _vars.add(arg1);
  _vars.add(arg2);
  _weights = w;
  initialise();
}

MipWrapper_Sum::MipWrapper_Sum(MipWrapper_Expression *arg, 
		   MipWrapperIntArray& w, 
		   const int offset)
  : MipWrapper_FloatVar() {
  _offset = offset;
  _vars.add(arg);
  _weights = w;
  initialise();
}

MipWrapper_Sum::MipWrapper_Sum()
  : MipWrapper_FloatVar(){
  _offset = 0;
}

void MipWrapper_Sum::initialise() {

  DBG("creating sum: size of params: %d\n", _vars.size());
  for(int i = 0; i < this->_vars.size(); ++i){
    int weight = this->_weights.get_item(i);
    if( weight > 0 ) {
      _lower += (weight * _vars.get_item(i)->_lower);
      _upper += (weight * _vars.get_item(i)->_upper);
    } else {
      _upper += (weight * _vars.get_item(i)->_lower);
      _lower += (weight * _vars.get_item(i)->_upper);
    }
  }
  _lower += _offset;
  _upper += _offset;
  DBG("Intermediate variable has domain [%f..%f]\n", _lower, _upper);    
}

MipWrapper_Sum::~MipWrapper_Sum(){}

void MipWrapper_Sum::addVar(MipWrapper_Expression* v){ _vars.add(v); }

void MipWrapper_Sum::addWeight(const int w){ _weights.add(w);}

void MipWrapper_Sum::set_rhs(const int k){ _offset = k; }

MipWrapper_Expression* MipWrapper_Sum::add(MipWrapperSolver *solver,
					   bool top_level){
  if(!this->has_been_added()){
    DBG("Adding sum constraint %s\n", "");
    //MipWrapper_Expression::add(solver, false);
    
    for(int i = 0; i < _vars.size(); ++i)
      _vars.set_item(i, _vars.get_item(i)->add(solver, false));
    
    if(!top_level){
      
      //LinearConstraint *con = new LinearConstraint(-_offset, -_offset);
      //con->add_coef(this, -1);
      //for(int i = 0; i < _vars.size(); ++i){
      //	_vars.set_item(i, _vars.get_item(i)->add(solver, false));
      //	con->add_coef(_vars.get_item(i), _weights.get_item(i));	
      //}
      //solver->_constraints.push_back(con);
      
    } else {
      std::cout << "Warning SUM constraint on top level not supporeted"
		<< std::endl;
      exit(1);
    }
  }
  return this;
}

/* Binary operators */

MipWrapper_binop::MipWrapper_binop(MipWrapper_Expression *var1,
				   MipWrapper_Expression *var2)
  : MipWrapper_FloatVar(){
  _vars[0] = var1;
  _vars[1] = var2;
  _is_proper_coef = false;
  _upper = 1.0;
  _lower = 0.0;
  DBG("Crearing binary operator %s\n", "");
}

 MipWrapper_binop::MipWrapper_binop(MipWrapper_Expression *var1, double rhs)
  : MipWrapper_FloatVar(){
  _vars[0] = var1;
  _vars[1] = NULL;
  _rhs = rhs;
  _is_proper_coef = true;
  _upper = 1.0;
  _lower = 0.0;
  DBG("Crearing binary operator %s\n", "");
}

MipWrapper_binop::~MipWrapper_binop(){}

MipWrapper_NoOverlap::MipWrapper_NoOverlap(MipWrapper_Expression *var1,
		      MipWrapper_Expression *var2,
		      MipWrapperIntArray &coefs):
  MipWrapper_binop(var1, var2), _coefs(coefs){}

MipWrapper_NoOverlap::~MipWrapper_NoOverlap(){}

MipWrapper_Expression* MipWrapper_NoOverlap::add(MipWrapperSolver *solver,
						 bool top_level){
  
  _vars[0] = _vars[0]->add(solver, false);
  _vars[1] = _vars[1]->add(solver, false);
  
  if(top_level){
    
    MipWrapperIntArray *arr1 = new MipWrapperIntArray();
    arr1->add(_coefs.get_item(0));
    solver->add_int_array(arr1);
    
    MipWrapperIntArray *arr2 = new MipWrapperIntArray();
    arr2->add(_coefs.get_item(1));
    solver->add_int_array(arr2);
    
    MipWrapper_Expression *prec1 = new MipWrapper_Precedence(_vars[0],
							     _vars[1], *arr1);
    solver->add_expr(prec1);
    
    MipWrapper_Expression *prec2 = new MipWrapper_Precedence(_vars[1],
							     _vars[0], *arr2);
    solver->add_expr(prec2);
    
    MipWrapper_Expression *orexp = new MipWrapper_or(prec1, prec2);
    solver->add_expr(orexp);
    orexp->add(solver, true);
    
  } else {
    std::cout << "No support for NoOverLap operator not in top level" << std::endl;
    exit(1);
  }
  return this;
}

MipWrapper_Precedence::MipWrapper_Precedence(MipWrapper_Expression *var1,
					     MipWrapper_Expression *var2,
					     MipWrapperIntArray &coefs):
  MipWrapper_binop(var1, var2), _coefs(coefs){}

MipWrapper_Precedence::~MipWrapper_Precedence(){}

MipWrapper_Expression* MipWrapper_Precedence::add(MipWrapperSolver *solver,
						  bool top_level){
  _vars[0] = _vars[0]->add(solver, false);
  _vars[1] = _vars[1]->add(solver, false);
  
  if(top_level){
    LinearConstraint *con = new LinearConstraint(-_coefs.get_item(0),INFINITY);
    con->add_coef(_vars[0], 1);
    con->add_coef(_vars[1], -1);
    solver->_constraints.push_back(con);
    return this;
  } else {
    std::cout << "Precedence at top level not supported yet" << std::endl;
    exit(1);
  }
  return this;
}

MipWrapper_eq::MipWrapper_eq(MipWrapper_Expression *var1,
			     MipWrapper_Expression *var2)
  : MipWrapper_binop(var1,var2){ DBG("creating equality constraint", NULL); }

MipWrapper_eq::MipWrapper_eq(MipWrapper_Expression *var1, double rhs)
  : MipWrapper_binop(var1,rhs){ DBG("creating equality constraint", NULL); }

MipWrapper_eq::~MipWrapper_eq(){}

MipWrapper_Expression* MipWrapper_eq::add(MipWrapperSolver *solver,
					  bool top_level){
  if(!has_been_added()){
    DBG("adding equality constraint", NULL);
    _vars[0] = _vars[0]->add(solver, false);
    
    if(top_level){
      if(_is_proper_coef){
	LinearConstraint *con = new LinearConstraint(_rhs, _rhs);
	con->add_coef(_vars[0], 1);
	solver->_constraints.push_back(con);
      } else {
	_vars[1] = _vars[1]->add(solver, false);
	LinearConstraint *con = new LinearConstraint(0, 0);
	con->add_coef(_vars[0], 1);
	con->add_coef(_vars[1], -1);
	solver->_constraints.push_back(con);
      }
    } else {
      _vars[0]->encode(solver);
      if(_is_proper_coef){
	if(_vars[0]->_expr_encoding[(int)_rhs] != NULL)
	  return _vars[0]->_expr_encoding[(int)_rhs];
	else return new MipWrapper_IntVar(0, 0);
      } else {
	MipWrapper_Expression *bvar = new MipWrapper_IntVar(0, 1);
	bvar = bvar->add(solver, false);
	_vars[1] = _vars[1]->add(solver, false);
	_vars[1]->encode(solver);
	
	int lb = (int) std::max(_vars[0]->_lower, _vars[1]->_lower);
	int ub = (int) std::min(_vars[0]->_upper, _vars[1]->_upper);
	
	// \forall_{i \in D_a \cup D_b} -1 <= A_i + B_i - C <= 1
	for(int i = lb; i <= ub; ++lb){
	  if(_vars[0]->_expr_encoding[i] != NULL &&
	     _vars[1]->_expr_encoding[i]){
	     LinearConstraint *con = new LinearConstraint(-1, 1);
	     con->add_coef(bvar, -1);
	     con->add_coef(_vars[0]->_expr_encoding[i], 1);
	     con->add_coef(_vars[1]->_expr_encoding[i], 1);
	     solver->_constraints.push_back(con);
	  }
	}
	
	double range_max = std::max( _vars[0]->_upper - _vars[1]->_lower,
				  _vars[1]->_upper - _vars[0]->_lower);
	MipWrapper_FloatVar *y = new MipWrapper_FloatVar(0, range_max);
	
	LinearConstraint *con = new LinearConstraint(1, INFINITY);
	con->add_coef(_vars[0], 1);
	con->add_coef(_vars[1], -1);
	con->add_coef(y, 1);
	solver->_constraints.push_back(con);
	
	con = new LinearConstraint(1, INFINITY);
	con->add_coef(_vars[0], -1);
	con->add_coef(_vars[1], 1);
	con->add_coef(y, 1);
	solver->_constraints.push_back(con);
	
	// RM <= C*RM + Y <= RM
	con = new LinearConstraint(range_max, range_max);
	con->add_coef(bvar, range_max);
	con->add_coef(y, 1);
	solver->_constraints.push_back(con);
	
	return bvar;
      }
    }
  }
  return this;
}

/* In equality operator */

MipWrapper_ne::MipWrapper_ne(MipWrapper_Expression *var1,
			     MipWrapper_Expression *var2)
  : MipWrapper_binop(var1,var2){}

MipWrapper_ne::MipWrapper_ne(MipWrapper_Expression *var1, double rhs)
  : MipWrapper_binop(var1,rhs){}

MipWrapper_ne::~MipWrapper_ne(){}

MipWrapper_Expression* MipWrapper_ne::add(MipWrapperSolver *solver,
					  bool top_level){
  if(!this->has_been_added()){
    DBG("adding not equal constraint %s\n", "");
    _vars[0] = _vars[0]->add(solver, false);
    _vars[0]->encode(solver);
    if(top_level){
      if(_is_proper_coef){
	if(_vars[0]->_expr_encoding[(int)_rhs] != NULL){
	  _vars[0]->_expr_encoding[(int)_rhs]->_lower = _rhs;
	  _vars[0]->_expr_encoding[(int)_rhs]->_upper = _rhs;
	}
      } else {
        _vars[1] = _vars[1]->add(solver, false);
        _vars[1]->encode(solver);
	
        int lb = std::max(this->_vars[0]->_lower, this->_vars[1]->_lower);
	int ub = std::min(this->_vars[0]->_upper, this->_vars[1]->_upper);
        
	for(int i = lb; i < ub; ++i)
	  if(_vars[0]->_expr_encoding[i] != NULL &&
	     _vars[1]->_expr_encoding[i] != NULL){
	    LinearConstraint *con = new LinearConstraint(0, 1);
	    con->add_coef(_vars[0]->_expr_encoding[i], 1);
	    con->add_coef(_vars[1]->_expr_encoding[i], 1);
	    solver->_constraints.push_back(con);
	  }
      }
      return this;
    } else {
      DBG("Reifing not equal constraint %s\n", "");
      
      if(_is_proper_coef){
	
	//TODO:
	
      } else {	
	_vars[1] = _vars[1]->add(solver, false);
	_vars[0]->encode(solver);
	_vars[1]->encode(solver);
      
	MipWrapper_Expression *c = new MipWrapper_IntVar(0, 1);
	c->add(solver, false);
	
	//TODO:
      
      }
    }
  }
  return this;
}

/* Leq operator */

MipWrapper_le::MipWrapper_le(MipWrapper_Expression *var1,
			     MipWrapper_Expression *var2)
  : MipWrapper_binop(var1,var2){ DBG("Crearing le constraint  %s\n", ""); }

MipWrapper_le::MipWrapper_le(MipWrapper_Expression *var1, double rhs)
  : MipWrapper_binop(var1,rhs){ DBG("Crearing le constraint  %s\n", ""); }

MipWrapper_le::~MipWrapper_le(){}

MipWrapper_Expression* MipWrapper_le::add(MipWrapperSolver *solver,
					  bool top_level){
  if(!has_been_added()){
    DBG("adding le constraint %s\n", "");
    _vars[0] = _vars[0]->add(solver, false);
    if(top_level){
      if(_is_proper_coef){
	_vars[0]->_upper = _rhs;
      } else {
	_vars[1] = _vars[1]->add(solver, false);
	LinearConstraint *con = new LinearConstraint(0, INFINITY);
	con->add_coef(_vars[0], -1);
	con->add_coef(_vars[1], 1);
	solver->_constraints.push_back(con);
      }
    } else {
      std::cout << "Leq operator not in top level insupported at the moment"
                << std::endl;
      exit(1);
    }
  }
  return this;
}

/* Geq operator */
MipWrapper_ge::MipWrapper_ge(MipWrapper_Expression *var1,
			     MipWrapper_Expression *var2)
  : MipWrapper_binop(var1,var2){ DBG("Crearing ge constraint  %s\n", ""); }

MipWrapper_ge::MipWrapper_ge(MipWrapper_Expression *var1,
			     double rhs)
  : MipWrapper_binop(var1,rhs){ DBG("Crearing ge constraint  %s\n", ""); }

MipWrapper_ge::~MipWrapper_ge(){}

MipWrapper_Expression* MipWrapper_ge::add(MipWrapperSolver *solver,
					  bool top_level){ 
  if(!has_been_added()){
    DBG("Adding ge constraint  %s\n", "");
    _vars[0] = _vars[0]->add(solver, false);
    if(top_level){
      if(this->_is_proper_coef){
	_vars[0]->_lower = _rhs;
      } else {
	_vars[1] = _vars[1]->add(solver, false);
	LinearConstraint *con = new LinearConstraint(0, INFINITY);
	con->add_coef(_vars[0], 1);
	con->add_coef(_vars[1], -1);
	solver->_constraints.push_back(con);
      }
      return this;
    } else {
      std::cerr << "Ge not at top level currently unsupported "
	        << std::endl;
      exit(1);
    }
  }
  return this;
}

/* Lt object */
MipWrapper_lt::MipWrapper_lt(MipWrapper_Expression *var1,
			     MipWrapper_Expression *var2)
  : MipWrapper_binop(var1,var2){ DBG("Crearing lt constraint %s\n", ""); }

MipWrapper_lt::MipWrapper_lt(MipWrapper_Expression *var1,
			     double rhs)
  : MipWrapper_binop(var1,rhs){ DBG("Crearing lt constraint %s\n", ""); }

MipWrapper_lt::~MipWrapper_lt(){}

MipWrapper_Expression* MipWrapper_lt::add(MipWrapperSolver *solver,
					  bool top_level){
  if(!has_been_added()){
    DBG("Adding lt constraint %s\n", "");
    _vars[0] = _vars[0]->add(solver, false);
    if(top_level){
      if(_is_proper_coef){
	_vars[0]->_upper = _rhs-1;
      } else {
	_vars[1] = _vars[1]->add(solver, false);
	LinearConstraint *con = new LinearConstraint(1, INFINITY);
	con->add_coef(_vars[0], -1);
	con->add_coef(_vars[1], 1);
	solver->_constraints.push_back(con);
      }
    } else {
      std::cout << "Lt operator not in top level insupported at the moment"
	        << std::endl;
      exit(1);
    }
  }
  return this;
}

/* Gt object */
MipWrapper_gt::MipWrapper_gt(MipWrapper_Expression *var1,
			     MipWrapper_Expression *var2)
  : MipWrapper_binop(var1,var2){ DBG("Crearing gt constraint %s\n", ""); }

MipWrapper_gt::MipWrapper_gt(MipWrapper_Expression *var1,
			     double rhs)
  : MipWrapper_binop(var1,rhs){ DBG("Crearing gt constraint %s\n", ""); }

MipWrapper_gt::~MipWrapper_gt(){}

MipWrapper_Expression* MipWrapper_gt::add(MipWrapperSolver *solver,
					  bool top_level){
  if(!has_been_added()){
    DBG("Adding gt constraint %s\n", ""); 
    _vars[0] = _vars[0]->add(solver, false);
    if(top_level){
      if(_is_proper_coef){
	_vars[0]->_lower = _rhs+1;
      } else {
	_vars[1] = _vars[1]->add(solver, false);
	LinearConstraint *con = new LinearConstraint(1, INFINITY);
	con->add_coef(_vars[0], 1);
	con->add_coef(_vars[1], -1);
	solver->_constraints.push_back(con);
      }
    } else {
      std::cout << "Gt operator not in top level insupported at the moment"
	        << std::endl;
      exit(1);
    }
  }
  return this;
}

/*
 *
 * Boolean operator stuff
 *
 */
MipWrapper_and::MipWrapper_and(MipWrapper_Expression *var1,
		    MipWrapper_Expression *var2):
  MipWrapper_binop(var1, var2){ DBG("Creating and constraint %s\n", ""); }

MipWrapper_and::~MipWrapper_and(){}

MipWrapper_Expression* MipWrapper_and::add(MipWrapperSolver *solver,
					   bool top_level){

  if(!has_been_added()){
    DBG("Adding in and constraint  %s\n", "");
    _vars[0] = _vars[0]->add(solver, false);
    _vars[1] = _vars[1]->add(solver, false);
    if(top_level){
      _vars[0]->_lower = 1;
      _vars[1]->_lower = 1;
    } else {
      std::cerr << "And constraint not in top level not supported "
		<< std::endl;
      exit(1);
    }
  }
  return this;
}

MipWrapper_or::MipWrapper_or(MipWrapper_Expression *var1,
			     MipWrapper_Expression *var2):
  MipWrapper_binop(var1, var2){ DBG("Creating or constraint %s\n", ""); }

MipWrapper_or::~MipWrapper_or(){}

MipWrapper_Expression* MipWrapper_or::add(MipWrapperSolver *solver,
					  bool top_level){

  if(!has_been_added()){
    DBG("Adding or constraint %s\n", "");
    _vars[0] = _vars[0]->add(solver, false);
    _vars[1] = _vars[1]->add(solver, false);
    if(top_level){
      LinearConstraint *con = new LinearConstraint(1, 2);
      con->add_coef(_vars[0], 1);
      con->add_coef(_vars[1], 1);
      solver->_constraints.push_back(con);
    } else {
      std::cerr << "Or constraint not in top level not supported "
		<< std::endl;
      exit(1);
    }
  }
  return this;
}

MipWrapper_not::MipWrapper_not(MipWrapper_Expression *var1):
  MipWrapper_binop(var1, 0.0){ DBG("Creating not constraint %s\n", ""); }

MipWrapper_not::~MipWrapper_not(){}

MipWrapper_Expression* MipWrapper_not::add(MipWrapperSolver *solver,
					   bool top_level){
  if(!has_been_added()){
    _vars[0] = _vars[0]->add(solver, false);
    if(top_level){
      _vars[0]->_upper = 0;
    } else {
      MipWrapper_Expression *c = new MipWrapper_IntVar(0, 1);
      c->add(solver, false);
      LinearConstraint *con = new LinearConstraint(1, 1);
      con->add_coef(_vars[0], 1);
      con->add_coef(c, 1);
      solver->_constraints.push_back(con);
      return c;
    }
  }
  return this;
} 

/**
 * Optimisation stuff
 */

// Minimise Class
MipWrapper_Minimise::MipWrapper_Minimise(MipWrapper_Expression *arg1):
MipWrapper_eq(arg1, new MipWrapper_FloatVar(arg1->_lower, arg1->_upper)){
  this->_vars[1]->_coef = -1;
}
MipWrapper_Minimise::~MipWrapper_Minimise(){}

// Maximise Class
MipWrapper_Maximise::MipWrapper_Maximise(MipWrapper_Expression *arg1):
MipWrapper_eq(arg1, new MipWrapper_FloatVar(arg1->_lower, arg1->_upper)){
  this->_vars[1]->_coef = 1;
}
MipWrapper_Maximise::~MipWrapper_Maximise(){}


/**************************************************************
 ********************     Solver        ***********************
 **************************************************************/

MipWrapperSolver::MipWrapperSolver(){
  DBG("Create a MIP solver %s\n", "");
  var_counter = 0;
}

MipWrapperSolver::~MipWrapperSolver(){ DBG("Delete a MIP Solver %s\n", "");}

void MipWrapperSolver::add(MipWrapper_Expression* arg){
   DBG("Adding an expression %s\n", "");
  if(arg != NULL) arg->add(this, true);
}

void MipWrapperSolver::add_expr(MipWrapper_Expression *expr){
  _exprs.push_back(expr);
}

void MipWrapperSolver::add_int_array(MipWrapperIntArray *arr){
  _int_array.push_back(arr);
}

void MipWrapperSolver::add_var_array(MipWrapperExpArray *arr){
  _var_array.push_back(arr);
}

void MipWrapperSolver::initialise(MipWrapperExpArray& arg){
  DBG("Initialise the Solver with search vars %s\n", "");
}

void MipWrapperSolver::initialise(){
  DBG("Initialise the Solver %s\n", "");
}

int MipWrapperSolver::solve(){
  DBG("Solve %s\n", "");
  
  std::cout << "c solve with mip wrapper " << std::endl;
  for(unsigned int i = 0; i < _constraints.size(); ++i)
    _constraints[i]->display();
  
  return SAT;
}

int MipWrapperSolver::solveAndRestart(const int policy,
				const unsigned int base,
				const double factor,
				const double decay ){
  return solve();
}

int MipWrapperSolver::startNewSearch(){
  std::cout << "c start a new interuptable search" << std::endl;
  return 0;
}

int MipWrapperSolver::getNextSolution(){
  std::cout << "c seek next solution" << std::endl;
  return 0;
}

int MipWrapperSolver::sacPreprocess(const int type){
  std::cout << "c enforces singleton arc consistency" << std::endl;
  return 0;
}

void MipWrapperSolver::setHeuristic(const char* var_heuristic,
				    const char* val_heuristic,
				    const int rand){
  std::cout << "c set the variable/value ordering (ignored)" << std::endl;
}

void MipWrapperSolver::setFailureLimit(const int cutoff){
  std::cout << "c set a cutoff on failures" << std::endl;
}

void MipWrapperSolver::setTimeLimit(const int cutoff){}

void MipWrapperSolver::setVerbosity(const int degree){
  _verbosity = degree;
}

void MipWrapperSolver::setRandomized(const int degree){
  std::cout << "c set the type of randomization" << std::endl;
}

void MipWrapperSolver::setRandomSeed(const int seed){
  std::cout << "c set the random seed" << std::endl;
}

bool MipWrapperSolver::is_sat(){
  return true;
}

bool MipWrapperSolver::is_unsat(){
  return ! is_sat();
}

void MipWrapperSolver::printStatistics(){
  std::cout << "\td Time: " << getTime() << "\tNodes:" << getNodes()
	    << std::endl;
}

int MipWrapperSolver::getBacktracks(){
  std::cout << "c print the number of backtracks" << std::endl;
  return 0;
}

int MipWrapperSolver::getNodes(){
  return 0;
}

int MipWrapperSolver::getFailures(){
  std::cout << "c print the number of failures" << std::endl;
  return 0;
}

int MipWrapperSolver::getChecks(){
  std::cout << "c print the number of checks" << std::endl;
  return 0;
}

int MipWrapperSolver::getPropags(){
  std::cout << "c print the number of propags" << std::endl;
  return 0;
}

double MipWrapperSolver::getTime(){
  return 0;
}

/**
 * Creates an empty linear constraint object
 */
LinearConstraint::LinearConstraint(double lhs, double rhs):
  _lhs(lhs), _rhs(rhs){}

LinearConstraint::~LinearConstraint(){}
    
void LinearConstraint::add_coef(MipWrapper_Expression* expr,
				double coef,
				bool use_encoding){
  DBG("Adding in expression to linear constraint type: %s\n",
      typeid(expr).name());
  if( use_encoding && expr->_expr_encoding != NULL){
    DBG("Adding in expression encoding %s\n", "");
    for(int i = expr->_lower; i <= expr->_upper; ++i )
      if(expr->_expr_encoding[i] != NULL) add_coef(expr->_expr_encoding[i],
						   i*coef);
  } else {
    _variables.push_back(expr);
    _coefficients.push_back(coef);
  }
}

void LinearConstraint::add_coef(MipWrapper_Sum* expr, double coef){
  DBG("Adding in sum to linear constraint %s\n", "");
  _lhs -= expr->_offset;
  _rhs -= expr->_offset;
  for(int i = 0; i < expr->_vars.size(); ++i)
    add_coef(expr->_vars.get_item(i), expr->_weights.get_item(i)*coef);
}

void LinearConstraint::add_coef(LinearView* expr, double coef){}
void LinearConstraint::add_coef(SumView* expr, double coef){}
void LinearConstraint::add_coef(ProductView* expr, double coef){}
    
/**
 * Prints the linear constraint
 */
void LinearConstraint::display(){
  std::cout << _lhs << " <= ";
  for(unsigned int i = 0; i < _variables.size(); ++i){
    std::cout << _coefficients[i] << "*X" << _variables[i]->_ident;
    if(i + 1 < _variables.size()) std::cout << " + ";
  }
  std::cout << " <= " << _rhs << std::endl;
}