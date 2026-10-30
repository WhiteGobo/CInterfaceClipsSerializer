#include "clips_script_serializer.h"
#include "raptor_serializer.h"
#include "info_query.h"
#include "crifi_raptor_interface.h"
#include "rdf_object.h"
#include "crifi_lists.h"

#define EX(suffix) "http://example.com/" suffix
#define CLIPS(suffix) "http://clips.script/" suffix
#define RDF(suffix) "http://www.w3.org/1999/02/22-rdf-syntax-ns#" suffix
#define RDFS(suffix) "http://www.w3.org/2000/01/rdf-schema#" suffix

enum {
	SAT_NOERROR = 0,
	SAT_SUBJ,
	SAT_PRED,
	SAT_OBJ,
	SAT_LISTITEMS,
	SAT_LISTIDFAILED,
	SAT_LISTELEMENTFAILED,
	SAT_SERIALIZEFAILED
};

typedef enum {
	uri,
	bnode
} IdType;

typedef struct {
	raptor_world* world;
	NodeList* nodes;
	RDFContext* rdf_cntxt;
	raptor_term *rdf_first;
	raptor_term *rdf_rest;
	raptor_term *rdf_nil;
	raptor_term *rdf_type;
	raptor_term *rdf_List;

	//used when creating a callable function
	raptor_term *ex_rootfunction;

	raptor_term *clips_Defrule;
	raptor_term *clips_rule_name;
	raptor_term *clips_salience;
	raptor_term *clips_conditional_element;
	raptor_term *clips_action;
	//raptor_term *clips_Pattern_CE
	
	raptor_term *clips_Deffacts;
	raptor_term *clips_deffacts_name;
	raptor_term *clips_rhs_pattern;
	
	raptor_term *clips_pattern_ce;
	raptor_term *clips_fact_var_symbol;
	raptor_term *clips_AssignedPatternCE;
	raptor_term *clips_TemplatePatternCE;
	raptor_term *clips_NotCE;
	raptor_term *clips_AndCE;
	raptor_term *clips_OrCE;
	raptor_term *clips_ExistsCE;
	raptor_term *clips_TestCE;
	raptor_term *clips_FunctionCall;
	raptor_term *clips_function_name;
	raptor_term *clips_function_args;
	raptor_term *clips_expression;
	raptor_term *clips_assert;
	raptor_term *clips_find_fact;
	raptor_term *clips_find_all_facts;
	raptor_term *clips_any_factp;
	raptor_term *clips_do_for_fact;
	raptor_term *clips_do_for_all_facts;
	raptor_term *clips_fact_set_template;
	raptor_term *clips_fact_set_member_variable;
	raptor_term *clips_member_variable;
	raptor_term *clips_member_slot_name;
	raptor_term *clips_query;
	raptor_term *clips_function_call;

	raptor_term *clips_deftemplate_name;
	raptor_term *clips_slot;
	raptor_term *clips_slot_name;
	raptor_term *clips_constraint;
	raptor_term *clips_constraints;
	raptor_term *clips_field;

	raptor_term *clips_symbol;
	raptor_term *clips_string;
	raptor_term *clips_MultifieldVariable;
	raptor_term *clips_Variable;
	raptor_term *clips_variable_name;
	raptor_term *clips_var_as_const_expr;
	raptor_term *clips_Function;

	raptor_term *clips_SingleWildcard;
	raptor_term *clips_MultiWildcard;
} MyContext;


static CRIFI_SERIALIZE_SCRIPT_RET add_info_to_tree_list(MyContext* cntxt, crifi_graph *graph, Fact *l);
static CRIFI_SERIALIZE_SCRIPT_RET add_info_to_tree_member(MyContext* cntxt, crifi_graph *graph, Fact *l);

static int fprintf_raptor_term(FILE* stream, raptor_term* term){
	switch(term->type){
		case RAPTOR_TERM_TYPE_LITERAL:
			fprintf(stream, "%s", term->value.literal.string);
			break;
		default:
			fprintf(stderr, "Missing support in fprinf_term "
					"for type\n");
			return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
	}
	return CRIFI_SERIALIZE_SCRIPT_NOERROR;
}

#define URI raptor_new_term_from_uri_string
static MyContext* init_context(){
	MyContext *cntxt = malloc(sizeof(MyContext));
	if (cntxt == NULL){
		return NULL;
	}
	raptor_world *world = raptor_new_world();
	cntxt->world = world;
	cntxt->nodes = new_nodelist();
	cntxt->rdf_cntxt = init_rdf_context(world);
	cntxt->rdf_type = raptor_new_term_from_uri_string(world, RDF("type"));
	cntxt->rdf_first = raptor_new_term_from_uri_string(world, RDF("first"));
	cntxt->rdf_rest = raptor_new_term_from_uri_string(world, RDF("rest"));
	cntxt->rdf_nil = raptor_new_term_from_uri_string(world, RDF("nil"));

	cntxt->ex_rootfunction = URI(world, EX("rootfunction"));

	cntxt->clips_Defrule = URI(world, CLIPS("Defrule"));
	cntxt->clips_rule_name = URI(world, CLIPS("rule-name"));
	cntxt->clips_salience = URI(world, CLIPS("salience"));

	cntxt->clips_Deffacts = URI(world, CLIPS("Deffacts"));
	cntxt->clips_deffacts_name = URI(world, CLIPS("deffacts-name"));
	cntxt->clips_rhs_pattern = URI(world, CLIPS("rhs-pattern"));

	cntxt->clips_conditional_element = URI(world, CLIPS("conditional-element"));
	cntxt->clips_action = URI(world, CLIPS("action"));

	cntxt->clips_pattern_ce = URI(world, CLIPS("pattern-ce"));
	cntxt->clips_fact_var_symbol = URI(world, CLIPS("fact-var-symbol"));
	cntxt->clips_AssignedPatternCE = URI(world, CLIPS("AssignedPatternCE"));
	cntxt->clips_TemplatePatternCE = URI(world, CLIPS("TemplatePatternCE"));
	cntxt->clips_NotCE = URI(world, CLIPS("NotCE"));
	cntxt->clips_AndCE = URI(world, CLIPS("AndCE"));
	cntxt->clips_OrCE = URI(world, CLIPS("OrCE"));
	cntxt->clips_ExistsCE = URI(world, CLIPS("ExistsCE"));
	cntxt->clips_TestCE = URI(world, CLIPS("TestCE"));
	cntxt->clips_FunctionCall = URI(world, CLIPS("FunctionCall"));

	cntxt->clips_deftemplate_name = URI(world, CLIPS("deftemplate-name"));
	cntxt->clips_slot = URI(world, CLIPS("slot"));
	cntxt->clips_slot_name = URI(world, CLIPS("slot-name"));
	cntxt->clips_constraint = URI(world, CLIPS("constraint"));
	cntxt->clips_constraints = URI(world, CLIPS("constraints"));
	cntxt->clips_field = URI(world, CLIPS("field"));
	cntxt->clips_SingleWildcard = URI(world, CLIPS("SingleWildcard"));
	cntxt->clips_MultiWildcard = URI(world, CLIPS("MultiWildcard"));

	cntxt->clips_function_name = URI(world, CLIPS("function-name"));
	cntxt->clips_function_args = URI(world, CLIPS("function-args"));

	cntxt->clips_assert = URI(world, CLIPS("assert"));
	cntxt->clips_find_fact = URI(world, CLIPS("FindFact"));
	cntxt->clips_find_all_facts = URI(world, CLIPS("FindAllFacts"));
	cntxt->clips_any_factp = URI(world, CLIPS("AnyFactP"));
	cntxt->clips_do_for_fact = URI(world, CLIPS("DoForFact"));
	cntxt->clips_do_for_all_facts = URI(world, CLIPS("DoForAllFacts"));
	cntxt->clips_fact_set_template = URI(world, CLIPS("fact-set-template"));
	cntxt->clips_fact_set_member_variable = URI(world, CLIPS("fact-set-member-variable"));
	cntxt->clips_member_variable = URI(world, CLIPS("member-variable"));
	cntxt->clips_member_slot_name = URI(world, CLIPS("member-slot-name"));
	cntxt->clips_query = URI(world, CLIPS("query"));
	cntxt->clips_function_call = URI(world, CLIPS("function-call"));

	//constant thingies
	cntxt->clips_symbol = URI(world, CLIPS("symbol"));
	cntxt->clips_string = URI(world, CLIPS("string"));
	
	cntxt->clips_MultifieldVariable = URI(world, CLIPS("MultifieldVariable"));
	cntxt->clips_Variable = URI(world, CLIPS("Variable"));
	cntxt->clips_variable_name = URI(world, CLIPS("variable-name"));
	cntxt->clips_var_as_const_expr = URI(world, CLIPS("var-as-const-expr"));
	return cntxt;
}


static void free_context(MyContext* cntxt){
	if (cntxt == NULL) return;
	raptor_free_term(cntxt->rdf_type);
	raptor_free_term(cntxt->rdf_first);
	raptor_free_term(cntxt->rdf_rest);
	raptor_free_term(cntxt->rdf_nil);
	raptor_free_term(cntxt->ex_rootfunction);
	raptor_free_term(cntxt->clips_Defrule);
	raptor_free_term(cntxt->clips_rule_name);
	raptor_free_term(cntxt->clips_salience);
	raptor_free_term(cntxt->clips_Deffacts);
	raptor_free_term(cntxt->clips_deffacts_name );
	raptor_free_term(cntxt->clips_rhs_pattern );
	raptor_free_term(cntxt->clips_conditional_element );
	raptor_free_term(cntxt->clips_action );
	raptor_free_term(cntxt->clips_pattern_ce );
	raptor_free_term(cntxt->clips_fact_var_symbol );
	raptor_free_term(cntxt->clips_AssignedPatternCE );
	raptor_free_term(cntxt->clips_TemplatePatternCE );
	raptor_free_term(cntxt->clips_NotCE );
	raptor_free_term(cntxt->clips_AndCE );
	raptor_free_term(cntxt->clips_OrCE );
	raptor_free_term(cntxt->clips_ExistsCE );
	raptor_free_term(cntxt->clips_TestCE );
	raptor_free_term(cntxt->clips_FunctionCall );
	raptor_free_term(cntxt->clips_deftemplate_name );
	raptor_free_term(cntxt->clips_slot );
	raptor_free_term(cntxt->clips_slot_name );
	raptor_free_term(cntxt->clips_constraint );
	raptor_free_term(cntxt->clips_constraints );
	raptor_free_term(cntxt->clips_field );
	raptor_free_term(cntxt->clips_SingleWildcard );
	raptor_free_term(cntxt->clips_MultiWildcard );
	raptor_free_term(cntxt->clips_function_name );
	raptor_free_term(cntxt->clips_function_args );
	raptor_free_term(cntxt->clips_assert );
	raptor_free_term(cntxt->clips_find_fact );
	raptor_free_term(cntxt->clips_find_all_facts );
	raptor_free_term(cntxt->clips_any_factp );
	raptor_free_term(cntxt->clips_do_for_fact );
	raptor_free_term(cntxt->clips_do_for_all_facts );
	raptor_free_term(cntxt->clips_fact_set_template );
	raptor_free_term(cntxt->clips_fact_set_member_variable );
	raptor_free_term(cntxt->clips_member_variable );
	raptor_free_term(cntxt->clips_member_slot_name );
	raptor_free_term(cntxt->clips_query );
	raptor_free_term(cntxt->clips_function_call );

	raptor_free_term(cntxt->clips_symbol );
	raptor_free_term(cntxt->clips_string );
	
	raptor_free_term(cntxt->clips_MultifieldVariable );
	raptor_free_term(cntxt->clips_Variable );
	raptor_free_term(cntxt->clips_variable_name);
	raptor_free_term(cntxt->clips_var_as_const_expr);

	free_nodelist(cntxt->nodes);
	free_rdf_context(cntxt->rdf_cntxt);
	raptor_free_world(cntxt->world);
	free(cntxt);
}


static CRIFI_SERIALIZE_SCRIPT_RET add_info_to_tree_list(MyContext* cntxt, crifi_graph *graph, Fact *l){
	int err, length;
	bool success;
	raptor_term *first = cntxt->rdf_cntxt->rdf_first;
	raptor_term *rest = cntxt->rdf_cntxt->rdf_rest;
	raptor_term *nil = cntxt->rdf_cntxt->rdf_nil;
	raptor_term *last = NULL;
	raptor_term *current = NULL;
	raptor_term *element = NULL;
	CLIPSValue l_val = {.factValue = l};
	CLIPSValue list_iterator_node = {.voidValue = VoidConstant(graph)};
	CLIPSValue *tmpval;// = {.voidValue = VoidConstant(graph)};
	Multifield *items;
	CLIPSValue clipsval_list = {.factValue = l};
	items = retrieve_items(graph, l_val);
	if (items == NULL){
		return SAT_LISTITEMS;
	}
	length = items->length;
	if (length == 0){ //no further serializing needed
		return SAT_NOERROR;
	}
	
	for (int i = 0; i<length; i++){
		if (!crifi_list_as_identifier(graph, &l_val, i, &list_iterator_node)){
			return SAT_LISTIDFAILED;
		}
		current = clipsvalue_to_raptorterm(cntxt->world, graph, list_iterator_node);
		if (current == NULL){
			return CRIFI_SERIALIZE_SCRIPT_OBJECT;
		}
		tmpval = items->contents + i;
		element = clipsvalue_to_raptorterm(cntxt->world, graph, *tmpval);
		if (element == NULL){
			return SAT_LISTELEMENTFAILED;
		}
		err = add_triple(cntxt->nodes, current, first, element);
		raptor_free_term(element);
		switch (err){
			case 0: break;
			default: return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
		}
		if (last != NULL){
			err = add_triple(cntxt->nodes, last, rest, current);
			raptor_free_term(last);
			switch (err){
				case 0: break;
				default: return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
			}
		}
		last = current;
		current = NULL;
	}
	success = true;
	if (last != NULL){
		err = add_triple(cntxt->nodes, last, rest, nil);
		raptor_free_term(last);
		last = NULL;
	}
	switch (err){
		case 0: break;
		default:
			return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
	}
	return CRIFI_SERIALIZE_SCRIPT_NOERROR;
}


static CRIFI_SERIALIZE_SCRIPT_RET add_info_to_tree(MyContext* cntxt,
							crifi_graph *graph)
{
	int err;
	if (cntxt == NULL || graph == NULL){
		return CRIFI_SERIALIZE_SCRIPT_CANT_CREATE_STRUCTS;
	}
	CLIPSValue tmpValue;
	raptor_statement *triple;
	raptor_term *subj, *pred, *obj;
	for(Fact *f = get_next_triple(graph, NULL);
			f != NULL;
			f = get_next_triple(graph, f))
	{
		GetFactSlot(f, TRIPLESLOTPREDICATE, &tmpValue);
		pred = clipsvalue_to_raptorterm(cntxt->world, graph, tmpValue);
		if (pred == NULL){
			return CRIFI_SERIALIZE_SCRIPT_PREDICATE;
		}

		GetFactSlot(f, TRIPLESLOTSUBJECT, &tmpValue);
		subj = clipsvalue_to_raptorterm(cntxt->world, graph, tmpValue);
		if (subj == NULL){
			raptor_free_term(pred);
			return CRIFI_SERIALIZE_SCRIPT_SUBJECT;
		}

		GetFactSlot(f, TRIPLESLOTOBJECT, &tmpValue);
		obj = clipsvalue_to_raptorterm(cntxt->world, graph, tmpValue);
		if (obj == NULL){
			raptor_free_term(subj);
			raptor_free_term(pred);
			return CRIFI_SERIALIZE_SCRIPT_OBJECT;
		}
		//triple = raptor_new_statement_from_nodes(world,
		//					subj, pred, obj, NULL);

		err = add_triple(cntxt->nodes, subj, pred, obj);
		raptor_free_term(subj);
		raptor_free_term(pred);
		raptor_free_term(obj);
		switch (err){
			case 0:
				break;
			default:
				return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
		}
		//raptor_serializer_serialize_statement(my_serializer, triple);
		//raptor_free_statement(triple);
	}
	for(Fact *l = get_next_list(graph, NULL);
			l != NULL;
			l = get_next_list(graph, l))
	{
		//fprintf(stderr, "convert list:\n");
		err = add_info_to_tree_list(cntxt, graph, l);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
			return err;
		}
	}
	for (Fact *member = get_next_member(graph, NULL);
			member != NULL;
			member = get_next_member(graph, member))
	{
		err = add_info_to_tree_member(cntxt, graph, member);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
			return err;
		}
	}
	return CRIFI_SERIALIZE_SCRIPT_NOERROR;
}


/* Parsing description */

#define FUNC_DESC(funcname) static CRIFI_SERIALIZE_SCRIPT_RET funcname(MyContext *cntxt, FILE* stream, Node* n)

FUNC_DESC(fprintf_lhs_slot);
FUNC_DESC(fprintf_assigned_pattern_ce);
FUNC_DESC(fprintf_template_pattern_ce);
FUNC_DESC(fprintf_conditional_element);
FUNC_DESC(fprintf_multi_ce);
FUNC_DESC(fprintf_variable);
FUNC_DESC(fprintf_variableslot);
FUNC_DESC(fprintf_expression);
FUNC_DESC(fprintf_action);
FUNC_DESC(fprintf_defrule);
FUNC_DESC(fprintf_deffacts);
FUNC_DESC(fprintf_constraint);
FUNC_DESC(fprintf_term);
FUNC_DESC(fprintf_constant);
FUNC_DESC(fprintf_function);
FUNC_DESC(fprintf_assert);
FUNC_DESC(fprintf_find_facts);

FUNC_DESC(fprintf_template_rhs_pattern);
FUNC_DESC(fprintf_rhs_slot);
FUNC_DESC(fprintf_rhs_field);
FUNC_DESC(fprintf_test_ce);


/**
 * working through CRIFI_SERIALIZE_BROKEN_GRAPH is safe
 */
FUNC_DESC(fprintf_constant){
	raptor_term *symbol, *string;
	symbol = get_object(n, cntxt->clips_symbol);
	if (symbol != NULL){
		fprintf(stream, " ");
		fprintf_raptor_term(stream, symbol);
		fprintf(stream, " ");
		return CRIFI_SERIALIZE_SCRIPT_NOERROR;
	}
	string = get_object(n, cntxt->clips_string);
	if (string != NULL){
		fprintf(stream, " ");
		fprintf_raptor_term(stream, string);
		fprintf(stream, " ");
		return CRIFI_SERIALIZE_SCRIPT_NOERROR;
	}
	return CRIFI_SERIALIZE_BROKEN_GRAPH;
}

/**
 * <term> ::= <constant> |
 * 		<single-field-variable> |
 * 		<multifield-variable> |
 * 		:<function-call> |
 * 		=<function-call>
 */
FUNC_DESC(fprintf_term){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	err = fprintf_constant(cntxt, stream, n);
	if (err != CRIFI_SERIALIZE_BROKEN_GRAPH) return err;
	err = fprintf_variable(cntxt, stream, n);
	if (err != CRIFI_SERIALIZE_BROKEN_GRAPH) return err;
	return fprintf_function(cntxt, stream, n);
}

/**
 * <function-call> ::= (<function-name> <expression>*)
 * (<function-name> ::= literal value of given term)
 * <expression> ::= <constant> | <variable> | <function-call>
 * <term> ::= <constant> |
 * 		<single-field-variable> |
 * 		<multifield-variable> |
 * 		:<function-call> |
 * 		=<function-call>
 * <RHS-field> ::= <variable> | <constant> | <function-call>
 * <test-CE> ::= (test <function-call>)
 * <query> ::= <global-variable> | <function-call>
 */
FUNC_DESC(fprintf_function){
	CRIFI_SERIALIZE_SCRIPT_RET err = CRIFI_SERIALIZE_SCRIPT_NOERROR;
	NodeIterator* n_iter;
	raptor_term *name, *args, *assert;
	assert = get_object(n, cntxt->clips_assert);
	raptor_term *rt = cntxt->clips_find_all_facts;
	if (assert != NULL){
		return fprintf_assert(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_find_all_facts)
		|| check_property(n, cntxt->rdf_type, cntxt->clips_find_fact)
		|| check_property(n, cntxt->rdf_type, cntxt->clips_any_factp))
	{
		return fprintf_find_facts(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_do_for_all_facts)
		|| check_property(n, cntxt->rdf_type, cntxt->clips_do_for_fact))
	{
		return fprintf_find_facts(cntxt, stream, n);
	}
	name = get_object(n, cntxt->clips_function_name);
	if (name == NULL){
		debug_fprintf_node(stderr, "tried as function but is "
				"missing name: ", n, "\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	fprintf(stream, " (");
	fprintf_raptor_term(stream, name);

	args = get_object(n, cntxt->clips_function_args);
	if(args == NULL){
		fprintf(stderr, "missing function args\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, args);
	if(n_iter == NULL){
		fprintf(stderr, "function args are not rdflist\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x = node_iterator_get_next(n_iter)){
		err = fprintf_expression(cntxt, stream, x);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
			//fprintf(stderr, "failed printing one function arg\n");
			debug_fprintf_node(stderr, "failed printing one "
					"function arg: ", x, "\n");
			break;
		}
		fprintf(stream, " ");
	}
	free_node_iterator(n_iter); n_iter = NULL;
	fprintf(stream, ")");
	return err;

}

FUNC_DESC(fprintf_test_ce){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	raptor_term *call;
	Node *call_n;
	call = get_object(n, cntxt->clips_function_call);
	call_n = retrieve_node(cntxt->nodes, call);
	fprintf(stream, "(test");
	err = fprintf_function(cntxt, stream, call_n);
	fprintf(stream, ")\n");
	return err;
}

/**
 * (find-all-facts <fact-set-template> <query>)
 * (any-factp <fact-set-template> <query>)
 * <fact-set-template> ::= (<fact-set-member-template>+)
 * <fact-set-member-template>
 * 	::= (<fact-set-member-variable> <deftemplate-restrictions>)
 * <fact-set-member-variable> ::= <single-field-variable>
 * <deftemplate-restrictions> ::= <deftemplate-name-expression>+
 * (<deftemplate-name-expression> == <deftemplate-name>)
 * (query == <boolean-expression> == <function>)
 *
 * TODO: Missing implementation for multiple deftemplate-name-expression
 */
FUNC_DESC(fprintf_find_facts){
	CRIFI_SERIALIZE_SCRIPT_RET err = CRIFI_SERIALIZE_SCRIPT_NOERROR;
	raptor_term *type, *var_templates, *query_facts, *tmpfactvar, *tmprestriction;
	Node *tmpfactvar_n, *tmprestriction_n, *query_facts_n;
	raptor_term *find_all_facts = cntxt->clips_find_all_facts;
	raptor_term *fact_set_template = cntxt->clips_fact_set_template;
	raptor_term *fact_set_member_variable = cntxt->clips_fact_set_member_variable;
	raptor_term *deftemplate_name = cntxt->clips_deftemplate_name;
	TermIterator* t_iter;

	type = get_object(n, cntxt->rdf_type);
	if (0 != raptor_term_equals(type, find_all_facts)){
		fprintf(stream, " (find-all-facts ");
	} else if (0 != raptor_term_equals(type, cntxt->clips_find_fact)){
		fprintf(stream, " (find-fact ");
	} else if (0 != raptor_term_equals(type, cntxt->clips_any_factp)){
		fprintf(stream, " (any-factp ");
	} else if (0 != raptor_term_equals(type, cntxt->clips_do_for_fact)){
		fprintf(stream, " (do-for-fact ");
	} else if (0 != raptor_term_equals(type, cntxt->clips_do_for_all_facts)) {
		fprintf(stream, " (do-for-all-facts ");
	} else {
		fprintf(stderr, "Failed find facts1\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	fprintf(stream, "(");
	t_iter = new_object_iterator(n, fact_set_template);
	Node *x;
	for(raptor_term* x_term = object_iterator_get(t_iter);
			x_term != NULL;
			x_term = object_iterator_get_next(t_iter)){
		x = retrieve_node(cntxt->nodes, x_term);
		fprintf(stream, "(");
		tmpfactvar = get_object(x, fact_set_member_variable);
		tmpfactvar_n = retrieve_node(cntxt->nodes, tmpfactvar);
		if(tmpfactvar_n == NULL){
			fprintf(stderr, "Failed find facts2. "
					"Missing fact_set_member_variable\n");
			err = CRIFI_SERIALIZE_BROKEN_GRAPH;
			break;
		}
		err = fprintf_variable(cntxt, stream, tmpfactvar_n);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
			fprintf(stderr, "Failed find facts3. Couldnt "
					"print fact_set_member_variable\n");
			break;
		}

		tmprestriction = get_object(x, deftemplate_name);
		if(tmprestriction == NULL){
			fprintf(stderr, "Failed find facts4. "
					"No deftemplate_name\n");
			err = CRIFI_SERIALIZE_BROKEN_GRAPH;
			break;
		}
		err = fprintf_raptor_term(stream, tmprestriction);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
			fprintf(stderr, "Failed find facts5.\n");
			break;
		}
		fprintf(stream, ")");
	}
	fprintf(stream, ")");
	free_object_iterator(t_iter);
	if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR ){
		return err;
	}

	query_facts = get_object(n, cntxt->clips_query);
	query_facts_n = retrieve_node(cntxt->nodes, query_facts);
	if(query_facts_n == NULL){
		fprintf(stderr, "Failed find facts6. Missing query.\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	err = fprintf_function(cntxt, stream, query_facts_n);
	switch(err){
		case CRIFI_SERIALIZE_SCRIPT_NOERROR:
			break;
		default:
			fprintf(stderr, "failed query with %d\n", err);
			debug_fprintf_node(stderr, "query element: ", query_facts_n, "\n");
	}

	NodeIterator* n_iter;
	raptor_term *actions;
	if (
			0 != raptor_term_equals(type, cntxt->clips_do_for_all_facts)
			|| 0 != raptor_term_equals(type, cntxt->clips_do_for_fact)
	   ){
		actions = get_object(n, cntxt->clips_action);
		if(actions == NULL){
			fprintf(stderr, "missing cs:action.\n");
			return CRIFI_SERIALIZE_BROKEN_GRAPH;
		}
		n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, actions);
		if(n_iter == NULL){
			fprintf(stderr, "cs:action should target rdflist\n");
			return CRIFI_SERIALIZE_BROKEN_GRAPH;
		}
		for(Node* x = node_iterator_get(n_iter);
				x != NULL;
				x = node_iterator_get_next(n_iter)){
			err = fprintf_function(cntxt, stream, x);
			if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
				debug_fprintf_node(stderr, "failed printing "
						"one action: ", x, "\n");
				break;
			}
			fprintf(stream, " ");
		}
		free_node_iterator(n_iter);
	}

	fprintf(stream, ") ");
	return err;
}

/**
 * (assert <RHS-pattern>+)
 */
FUNC_DESC(fprintf_assert){
	CRIFI_SERIALIZE_SCRIPT_RET err = CRIFI_SERIALIZE_SCRIPT_NOERROR;
	raptor_term *assert;
	Node *assert_n;
	assert = get_object(n, cntxt->clips_assert);
	if (assert == NULL){
		fprintf(stderr, "fprintf_assert failed to find assert target\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	assert_n = retrieve_node(cntxt->nodes, assert);
	fprintf(stream, "(assert");
	err = fprintf_template_rhs_pattern(cntxt, stream, assert_n);
	fprintf(stream, ")");
	return err;
}

static CRIFI_SERIALIZE_SCRIPT_RET fprintf_lhs_slot(MyContext *cntxt, FILE* stream, Node* n){
	CRIFI_SERIALIZE_SCRIPT_RET err = CRIFI_SERIALIZE_SCRIPT_NOERROR;
	raptor_term *name, *constraint, *constraints;
	Node* constraint_n;
	NodeIterator* n_iter;
	name = get_object(n, cntxt->clips_slot_name);
	if (name == NULL) return CRIFI_SERIALIZE_BROKEN_GRAPH;
	fprintf(stream, " (");
	fprintf_raptor_term(stream, name);
	fprintf(stream, " ");

	constraint = get_object(n, cntxt->clips_constraint);
	if (constraint != NULL){
		constraint_n = retrieve_node(cntxt->nodes, constraint);
		if (constraint_n == NULL) return CRIFI_SERIALIZE_BROKEN_GRAPH;
		fprintf_constraint(cntxt, stream, constraint_n);
		//fprintf_raptor_term(stream, constraint);
	} else {
		constraints = get_object(n, cntxt->clips_constraints);
		if (constraints == NULL) return CRIFI_SERIALIZE_BROKEN_GRAPH;
		n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, constraints);
		for(Node* x = node_iterator_get(n_iter);
				x != NULL;
				x = node_iterator_get_next(n_iter)){
			err = fprintf_constraint(cntxt, stream, x);
			if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR) break;
		}
		free_node_iterator(n_iter);
	}
	fprintf(stream, ")");
	return err;
}

/**
 * <RHS-pattern> ::= <ordered-RHS-pattern> | <template-RHS-pattern>
 * <template-RHS-pattern> ::= (<deftemplate-name> <RHS-slot>*)
 */
static CRIFI_SERIALIZE_SCRIPT_RET fprintf_template_rhs_pattern(MyContext *cntxt, FILE* stream, Node* n){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	raptor_term *name, *slots;
	NodeIterator* n_iter;
	name = get_object(n, cntxt->clips_deftemplate_name);
	fprintf(stream, "(");
	if (name == NULL){
		fprintf(stderr, "template_rhs_pattern is missing a name\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	fprintf_raptor_term(stream, name);
	slots = get_object(n, cntxt->clips_slot);
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, slots);
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x=node_iterator_get_next(n_iter)){
		err = fprintf_rhs_slot(cntxt, stream, x);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR) break;
	}
	free_node_iterator(n_iter);

	fprintf(stream, ")");
	return err;
}

/**
 * <RHS-slot> ::= <single-field-RHS-slot> |
 * 		<multifield-RHS-slot>
 * <single-field-RHS-slot> ::= (<slot-name> <RHS-field>)
 * <multifield-RHS-slot> ::= (<slot-name> <RHS-field>*)
 */
static CRIFI_SERIALIZE_SCRIPT_RET fprintf_rhs_slot(MyContext *cntxt, FILE* stream, Node* n){
	//debug_fprintf_node(stderr, "start fprintf_rhs_slot ", n, "\n");
	CRIFI_SERIALIZE_SCRIPT_RET err = CRIFI_SERIALIZE_SCRIPT_NOERROR;
	int i = 0;
	raptor_term *name, *field, *fields;
	Node* constraint_n;
	NodeIterator* n_iter;
	name = get_object(n, cntxt->clips_slot_name);
	if (name == NULL){
		fprintf(stderr, "fprintf_rhs_slot is missing a cs:slot-name\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	fprintf(stream, " (");
	fprintf_raptor_term(stream, name);
	fprintf(stream, " ");

	fields = get_object(n, cntxt->clips_field);
	if(fields == NULL){
		fprintf(stderr, "fprintf_rhs_slot is missing cs:field\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, fields);
	if(n_iter == NULL){
		fprintf(stderr, "cs:field should target a rdf:List\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	i = 0;
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x = node_iterator_get_next(n_iter)){
		i++;
		err = fprintf_rhs_field(cntxt, stream, x);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR) break;
	}
	/*
	if (i == 0){
		fprintf(stderr, "qwertz found empty cs:field\n");
		//fprintf(stderr, "list for cs:field must be non empty.\n");
		//return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}*/
	free_node_iterator(n_iter);
	fprintf(stream, ")");
	//debug_fprintf_node(stderr, "end fprintf_rhs_slot ", n, "\n");
	return err;
}

/**
 * <RHS-field> ::= <variable> | <constant> | <function-call>
 */
FUNC_DESC(fprintf_rhs_field){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	err = fprintf_constant(cntxt, stream, n);
	switch (err){
		case CRIFI_SERIALIZE_SCRIPT_NOERROR:
			return CRIFI_SERIALIZE_SCRIPT_NOERROR;
		case CRIFI_SERIALIZE_BROKEN_GRAPH:
			break;
		default:
			fprintf(stderr, "fprintf_constant failed\n");
			return err;
	}
	err = fprintf_variable(cntxt, stream, n);
	switch (err){
		case CRIFI_SERIALIZE_SCRIPT_NOERROR:
			return CRIFI_SERIALIZE_SCRIPT_NOERROR;
		case CRIFI_SERIALIZE_BROKEN_GRAPH:
			break;
		default:
			fprintf(stderr, "fprintf_variable failed\n");
			return err;
	}
	return fprintf_function(cntxt, stream, n);
}


FUNC_DESC(fprintf_assigned_pattern_ce){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	raptor_term *pattern, *var;
	Node *pattern_n, *var_n;
	var = get_object(n, cntxt->clips_fact_var_symbol);
	var_n = retrieve_node(cntxt->nodes, var);
	if (var_n == NULL){
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	err = fprintf_variable(cntxt, stream, var_n);
	if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
		return err;
	}

	fprintf(stream, " <- ");

	pattern = get_object(n, cntxt->clips_pattern_ce);
	pattern_n = retrieve_node(cntxt->nodes, pattern);
	if (pattern_n == NULL){
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	if (check_property(pattern_n, cntxt->rdf_type, cntxt->clips_TemplatePatternCE)){
		return fprintf_template_pattern_ce(cntxt, stream, pattern_n);
	}
	return CRIFI_SERIALIZE_BROKEN_GRAPH;
}

static CRIFI_SERIALIZE_SCRIPT_RET fprintf_template_pattern_ce(MyContext *cntxt, FILE* stream, Node* n){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	raptor_term *name, *slots;
	NodeIterator* n_iter;
	name = get_object(n, cntxt->clips_deftemplate_name);
	fprintf(stream, "(");
	if (name == NULL) return CRIFI_SERIALIZE_BROKEN_GRAPH;
	fprintf_raptor_term(stream, name);
	slots = get_object(n, cntxt->clips_slot);
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, slots);
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x=node_iterator_get_next(n_iter)){
		err = fprintf_lhs_slot(cntxt, stream, x);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR) return err;
	}
	free_node_iterator(n_iter);

	fprintf(stream, ")");
	return CRIFI_SERIALIZE_SCRIPT_NOERROR;
}

/**
 * <conditional-element> ::= <pattern-CE> |
 * 			<assigned-pattern-CE> |
 * 			<not-CE> |
 * 			<and-CE> |
 * 			<or-CE> |
 * 			<logical-CE> |
 * 			<test-CE> |
 * 			<exists-CE> |
 * 			<forall-CE>
 */
static CRIFI_SERIALIZE_SCRIPT_RET fprintf_conditional_element(MyContext *cntxt, FILE* stream, Node* n){
	if (check_property(n, cntxt->rdf_type, cntxt->clips_AssignedPatternCE)){
		return fprintf_assigned_pattern_ce(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_TemplatePatternCE)){
		return fprintf_template_pattern_ce(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_TestCE)){
		return fprintf_test_ce(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_NotCE)){
		return fprintf_multi_ce(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_AndCE)){
		return fprintf_multi_ce(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_ExistsCE)){
		return fprintf_multi_ce(cntxt, stream, n);
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_OrCE)){
		return fprintf_multi_ce(cntxt, stream, n);
	}
	debug_fprintf_node(stderr, "failed conditional element ", n, "\n");
	return CRIFI_SERIALIZE_BROKEN_GRAPH;
}

FUNC_DESC(fprintf_multi_ce){
	CRIFI_SERIALIZE_SCRIPT_RET err = CRIFI_SERIALIZE_SCRIPT_NOERROR;
	raptor_term *targets;
	NodeIterator* n_iter;
	bool at_least_one = false;

	if (check_property(n, cntxt->rdf_type, cntxt->clips_NotCE)){
		fprintf(stream, "(not\n");
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_AndCE)){
		fprintf(stream, "(and\n");
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_OrCE)){
		fprintf(stream, "(or\n");
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_ExistsCE)){
		fprintf(stream, "(exists\n");
	} else {
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	targets = get_object(n, cntxt->clips_conditional_element);
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, targets);
	if(n_iter == NULL){
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x=node_iterator_get_next(n_iter)){
		at_least_one = true;
		err = fprintf_conditional_element(cntxt, stream, x);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
			break;
		}
		fprintf(stream, "\n");
	}
	free_node_iterator(n_iter);
	if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR && !at_least_one){
		err = CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	//err = fprintf_conditional_element(cntxt, stream, target_n);
	fprintf(stream, ")");
	return err;
}


/**
 * working through CRIFI_SERIALIZE_BROKEN_GRAPH is safe.
 * <global-variable> and <multifield-variable> are not implemented.
 *
 * <term> ::= <constant> |
 * 		<single-field-variable> |
 * 		<multifield-variable> |
 * 		:<function-call> |
 * 		=<function-call>
 * <expression> ::= <constant> || <variable> || <function-call>
 * <variable> ::= <single-field-variable> |
 * 		<multifield-variable> |
 * 		<global-variable>
 */
static CRIFI_SERIALIZE_SCRIPT_RET fprintf_variable(MyContext *cntxt, FILE* stream, Node* n){
	Node *replacement_n;
	raptor_term *varname, *replacement;
	replacement = get_object(n, cntxt->clips_var_as_const_expr);
	if (replacement != NULL){
		replacement_n = retrieve_node(cntxt->nodes, replacement);
		if (replacement_n == NULL){
			return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
		}
		return fprintf_expression(cntxt, stream, replacement_n);
	}
	varname = get_object(n, cntxt->clips_variable_name);
	if (varname != NULL){
		if (check_property(n, cntxt->rdf_type,
					cntxt->clips_MultifieldVariable)){
			fprintf(stream, " $?");
		} else {
			fprintf(stream, " ?");
		}
		fprintf_raptor_term(stream, varname);
		fprintf(stream, " ");
		return CRIFI_SERIALIZE_SCRIPT_NOERROR;
	}
	return CRIFI_SERIALIZE_BROKEN_GRAPH;
}

/**
 * working through CRIFI_SERIALIZE_BROKEN_GRAPH is safe.
 */
FUNC_DESC(fprintf_variableslot){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	Node *varnode_n;
	raptor_term *varnode, *slotname, *varname, *rt;
	varnode = get_object(n, cntxt->clips_member_variable);
	slotname = get_object(n, cntxt->clips_member_slot_name);
	//if (varnode == NULL) fprintf(stderr, "missing varnode\n");
	//if (slotname == NULL) fprintf(stderr, "missing slotname\n");

	if (varnode == NULL || slotname == NULL){
		if (varnode != NULL || slotname != NULL){
			debug_fprintf_node(stderr, "variable slot is missing information: ", n, "\n");
		}
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	varnode_n = retrieve_node(cntxt->nodes, varnode);
	varname = get_object(varnode_n, cntxt->clips_variable_name);
	if (varname == NULL){
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	fprintf(stream, " ?");
	fprintf_raptor_term(stream, varname);
	fprintf(stream, ":");
	fprintf_raptor_term(stream, slotname);
	fprintf(stream, " ");
	
	return CRIFI_SERIALIZE_SCRIPT_NOERROR;
}


/**
 * <constraint> ::= ? | $? | <connected-constraint>
 * <connected-constraint> ::= 
 * 			<single-constraint> |
 * 			<single-constraint> & <connected-constraint> |
 * 			<single-constraint> | <connected-constraint>
 * <single-constraint> ::= <term> | ~<term>
 */
FUNC_DESC(fprintf_constraint){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	Node *replacement_n;
	raptor_term *replacement;
	if (check_property(n, cntxt->rdf_type, cntxt->clips_SingleWildcard)){
		fprintf(stream, " ? ");
		return CRIFI_SERIALIZE_SCRIPT_NOERROR;
	} else if (check_property(n, cntxt->rdf_type, cntxt->clips_MultiWildcard)){
		fprintf(stream, " $? ");
		return CRIFI_SERIALIZE_SCRIPT_NOERROR;
	} else if (NULL != get_object(n, cntxt->clips_function_name)){
		fprintf(stream, " =");
		return fprintf_function(cntxt, stream, n);
	}
	replacement = get_object(n, cntxt->clips_var_as_const_expr);
	if (replacement != NULL){
		replacement_n = retrieve_node(cntxt->nodes, replacement);
		if (replacement_n == NULL){
			return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
		}
		return fprintf_constraint(cntxt, stream, replacement_n);
	}
	//no connected constraint implemented
	//return fprintf_term(cntxt, stream, n);
	err = fprintf_constant(cntxt, stream, n);
	if (err != CRIFI_SERIALIZE_BROKEN_GRAPH) return err;
	return fprintf_variable(cntxt, stream, n);
}

/**
 * <action> ::= <expression>
 * <function-call> ::= (<function-name> <expression>*)
 * <expression> ::= <constant> || <variable> || <function-call> ||
 * 		<instance-set-member-variable>:<slot-name> #use only in query
 */
static CRIFI_SERIALIZE_SCRIPT_RET fprintf_expression(MyContext *cntxt, FILE* stream, Node* n){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	err = fprintf_constant(cntxt, stream, n);
	switch(err){
		case CRIFI_SERIALIZE_BROKEN_GRAPH:
			break; //fprintf_constant is safe to work through
		case CRIFI_SERIALIZE_SCRIPT_NOERROR:
		default:
			return err;
	}
	err = fprintf_variable(cntxt, stream, n);
	switch(err){
		case CRIFI_SERIALIZE_BROKEN_GRAPH:
			break; //fprintf_variable is safe to work through
		case CRIFI_SERIALIZE_SCRIPT_NOERROR:
		default:
			return err;
	}
	err = fprintf_variableslot(cntxt, stream, n);
	switch(err){
		case CRIFI_SERIALIZE_BROKEN_GRAPH:
			break; //fprintf_variableslot is safe to work through
		case CRIFI_SERIALIZE_SCRIPT_NOERROR:
		default:
			return err;
	}
	err = fprintf_function(cntxt, stream, n);
	switch(err){
		case CRIFI_SERIALIZE_SCRIPT_NOERROR:
			break;
		default:
			fprintf(stderr, "failed function with %d\n", err);
			debug_fprintf_node(stderr, "function element: ", n, "\n");
	}
	return err;
}

static CRIFI_SERIALIZE_SCRIPT_RET fprintf_action(MyContext *cntxt, FILE* stream, Node* n){
	return fprintf_expression(cntxt, stream, n);
}


/**
 *
 * <defrule-construct> ::= (defrule <rule-name> [<comment>]
 * 			[<declaration>]
 * 			<conditional-element>*
 * 			=>
 * 			<action>*)
 * (<rule-name> ::= literal value of given term)
 * <action> ::= <expression>
 */
static CRIFI_SERIALIZE_SCRIPT_RET fprintf_defrule(MyContext *cntxt, FILE* stream, Node* n){

	CRIFI_SERIALIZE_SCRIPT_RET err;
	NodeIterator* n_iter;
	TermIterator* iter;
	raptor_term *name, *condition, *action, *entry, *salience;
	Node *tmpnode = NULL;

	fprintf(stream, "(defrule ");
	name = get_object(n, cntxt->clips_rule_name);
	if (name != NULL){
		fprintf_raptor_term(stream, name);
	} else {
		fprintf(stream, "rule%x", n);
	}
	salience = get_object(n, cntxt->clips_salience);
	if (salience != NULL){
		fprintf(stream, " (declare (salience ");
		fprintf_raptor_term(stream, salience);
		fprintf(stream, "))");
	}
	fprintf(stream, "\n");
	condition = get_object(n, cntxt->clips_conditional_element);
	if(condition == NULL){
		fprintf(stderr, "Missing cs:conditional-element.\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, condition);
	if(n_iter == NULL){
		fprintf(stderr, "cs:conditional-element should target a rdf:List\n");
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x=node_iterator_get_next(n_iter)){
		err = fprintf_conditional_element(cntxt, stream, x);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR) return err;
		fprintf(stream, "\n");
	}
	free_node_iterator(n_iter); n_iter = NULL;

	fprintf(stream, "\t=>\n");

	action = get_object(n, cntxt->clips_action);
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, action);
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x=node_iterator_get_next(n_iter)){
		err = fprintf_action(cntxt, stream, x);
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR) return err;
	}
	free_node_iterator(n_iter); n_iter = NULL;
	fprintf(stream, ")");
	return CRIFI_SERIALIZE_SCRIPT_NOERROR;
}

/**
 * ::= (deffacts <deffacts-name> [<comment>] <RHS-pattern>*)
 */
FUNC_DESC(fprintf_deffacts){
	CRIFI_SERIALIZE_SCRIPT_RET err = CRIFI_SERIALIZE_SCRIPT_NOERROR;
	NodeIterator* n_iter;
	raptor_term *rhspatterns, *name, *args, *assert;
	fprintf(stream, "(deffacts ");
	name = get_object(n, cntxt->clips_deffacts_name);
	if (name != NULL){
		fprintf_raptor_term(stream, name);
	} else {
		fprintf(stream, "facts%x", n);
	}
	fprintf(stream, "\n");

	rhspatterns = get_object(n, cntxt->clips_rhs_pattern);
	n_iter = new_rdflist_iterator(cntxt->rdf_cntxt, cntxt->nodes, rhspatterns);
	if(n_iter == NULL){
		return CRIFI_SERIALIZE_BROKEN_GRAPH;
	}
	for(Node* x = node_iterator_get(n_iter);
			x != NULL;
			x = node_iterator_get_next(n_iter)){
		err = fprintf_template_rhs_pattern(cntxt, stream, x);
		fprintf(stream, "\n");
		if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
			break;
		}
	}
	free_node_iterator(n_iter);
	fprintf(stream, ")");
	return err;
}


static CRIFI_SERIALIZE_SCRIPT_RET fprintf_script(MyContext *cntxt, FILE* stream){
	CRIFI_SERIALIZE_SCRIPT_RET err;
	NodeIterator *iter_node = new_node_iterator(cntxt->nodes);
	//If cntxt is initialized correctly this should never happen.
	if (iter_node == NULL){
		return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
	}
	for(Node* n = node_iterator_get(iter_node);
			n != NULL;
			n = node_iterator_get_next(iter_node)){
		if (check_property(n, cntxt->rdf_type, cntxt->clips_Defrule)){
			err = fprintf_defrule(cntxt, stream, n);
			if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
				return err;
			}
			fprintf(stream, "\n\n");
		} else if (check_property(n, cntxt->rdf_type, cntxt->clips_Deffacts)){
			err = fprintf_deffacts(cntxt, stream, n);
			if (err != CRIFI_SERIALIZE_SCRIPT_NOERROR){
				return err;
			}
			fprintf(stream, "\n\n");
		}
	}
	free_node_iterator(iter_node);
	return CRIFI_SERIALIZE_SCRIPT_NOERROR;
}

/* main method */

CRIFI_SERIALIZE_SCRIPT_RET serialize_information_as_clips_script(FILE* stream, crifi_graph* graph){
	int err = 0;
	if (stream == NULL || graph == NULL){
		return CRIFI_SERIALIZE_SCRIPT_INPUT;
	}
	MyContext *cntxt = init_context();
	if (cntxt == NULL){
		return CRIFI_SERIALIZE_SCRIPT_CANT_CREATE_STRUCTS;
	}
	err = add_info_to_tree(cntxt, graph);
	if (err == 0){
		err = fprintf_script(cntxt, stream);
	}
	free_context(cntxt);
	return err;
}

CRIFI_SERIALIZE_SCRIPT_RET serialize_information_as_clips_function(FILE* stream, crifi_graph* graph){
	int err = 0;
	int serialize_err = 0;
	if (stream == NULL || graph == NULL){
		return CRIFI_SERIALIZE_SCRIPT_INPUT;
	}
	MyContext *cntxt = init_context();
	if (cntxt == NULL){
		return CRIFI_SERIALIZE_SCRIPT_CANT_CREATE_STRUCTS;
	}
	err = add_info_to_tree(cntxt, graph);
	if (err != 0){
		free_context(cntxt);
		return err;
	}

	Node *rootfunction_n = retrieve_node(cntxt->nodes,
					cntxt->ex_rootfunction);
	if (rootfunction_n == NULL){
		free_context(cntxt);
		return CRIFI_SERIALIZE_MISSING_ROOT_FUNCTION;
	}
	serialize_err = fprintf_function(cntxt, stream, rootfunction_n);

	//raptor_free_term(cntxt->ex_rootfunction);
	free_context(cntxt);
	return serialize_err;
}


static CRIFI_SERIALIZE_SCRIPT_RET add_info_to_tree_member(MyContext* cntxt, crifi_graph *graph, Fact *member){
	int err;
	CLIPSValue clips_member = {.factValue=member};
	CLIPSValue instance, cls;
	raptor_term *type = cntxt->rdf_cntxt->rdf_type;
	raptor_term *term_instance = NULL;
	raptor_term *term_class = NULL;
	if (type == NULL){
		return SAT_SERIALIZEFAILED;
	}
	if(!crifi_infoquery_unpack_member(graph, clips_member, &instance, &cls))
	{
		return SAT_SERIALIZEFAILED;
	}
	term_instance = clipsvalue_to_raptorterm(cntxt->world, graph, instance);
	term_class = clipsvalue_to_raptorterm(cntxt->world, graph, cls);
	if (term_instance == NULL || term_class == NULL){
		return SAT_SERIALIZEFAILED;
	}

	err = add_triple(cntxt->nodes, term_instance, type, term_class);
	raptor_free_term(term_instance);
	raptor_free_term(term_class);
	switch (err){
		case 0: break;
		default: return CRIFI_SERIALIZE_SCRIPT_UNKNOWN;
	}

	return 0;
}
