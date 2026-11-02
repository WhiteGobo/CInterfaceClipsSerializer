use std::io::Error;
use oxigraph::store::Store;
use oxigraph::model::vocab::rdf;
use crate::constants::clips;
use crate::error::CICSError;
use uuid::Uuid;
use oxigraph::model::{NamedNodeRef, NamedOrBlankNodeRef, NamedOrBlankNode, Term, Literal, TermRef};

fn inset(input: String) -> String {
    format!("\t{}", &input.replace("\n", "\n\t"))
}

fn get_proplist(store: &Store, node: NamedOrBlankNodeRef, pred: NamedNodeRef) -> Option<Vec<NamedOrBlankNode>> {
    let listbase = match query_obj(store, node, pred){
        None => {return None;},
        Some(x) => x,
    };
    match TryInto::<NamedOrBlankNode>::try_into(listbase) {
        Err(_e) => None,
        Ok(x) => get_idnodelist(store, x.as_ref()),
    }
}

fn get_idnodelist(store: &Store, node: NamedOrBlankNodeRef) -> Option<Vec<NamedOrBlankNode>> {
    let rdf_nil: NamedOrBlankNodeRef = rdf::NIL.into();
    let mut ret: Vec<NamedOrBlankNode> = Vec::new();
    let mut x: NamedOrBlankNode = node.into();
    while x.as_ref() != rdf_nil {
        match query_obj(store, x.as_ref(), rdf::FIRST){
            None => {return None;},
            Some(first) => {ret.push(first);},
        }
        x = match query_obj(store, x.as_ref(), rdf::REST) {
            Some(new_base) => new_base,
            None => {return None;},
        };
    }
    Some(ret)
}

fn query_obj(store: &Store, node: NamedOrBlankNodeRef, pred: NamedNodeRef) -> Option<NamedOrBlankNode>
{
    match query_term(store, node, pred) {
        Some(term) => match TryInto::<NamedOrBlankNode>::try_into(term) {
            Ok(ret) => Some(ret),
            Err(_e) => None,
        },
        None => None,
    }
}

fn query_literal(store: &Store, node: NamedOrBlankNodeRef, pred: NamedNodeRef) -> Option<Literal>
{
    match query_term(store, node, pred) {
        Some(term) => {
            match TryInto::<Literal>::try_into(term) {
            Ok(ret) => Some(ret),
            Err(_e) => None,
        }},
        None => None,
    }
}

fn query_term(store: &Store, node: NamedOrBlankNodeRef, pred: NamedNodeRef) -> Option<Term>
{
    let q = store.quads_for_pattern(Some(node), Some(pred), None, None);
    for x in q {
        match x {
            Ok(y) => {return Some(y.object);},
            Err(_e) => {return None;},
        }
    }
    None
}

pub fn store2clips(store: &Store) -> Result<Vec<u8>, Error> {
    let mut ret = String::new();
    let q = store.quads_for_pattern(None, Some(rdf::TYPE), Some(clips::Defrule.into()), None);
    for q_quad in q {
        match q_quad {
            Err(_e) => {},
            Ok(quad) => match print_defrule(store, quad.subject.as_ref()){
                Some(x) => ret.push_str(&x),
                None => {return Err(Error::other("defrule"));},
            },
        }
        ret.push_str("\n\n");
    }
    let q2 = store.quads_for_pattern(None, Some(rdf::TYPE), Some(clips::Deffacts.into()), None);
    for q_quad in q2 {
        match q_quad {
            Err(_e) => {},
            Ok(quad) => match print_deffacts(store, quad.subject.as_ref()){
                Some(x) => ret.push_str(&x),
                None => {return Err(Error::other("deffacts"));},
            },
        }
        ret.push_str("\n\n");
    }
    Ok(ret.into_bytes())
}

fn print_defrule(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let name = match query_literal(store, node, clips::rule_name) {
        Some(x) => x.value().to_owned(),
        None => format!("rule{}", Uuid::new_v4()),
    };
    let mut extras = String::new();
    match get_salience(store, node){
        None => {},
        Some(x) => {
            extras.push_str(&format!("(declare (salience {}))", x));
        }
    }
    let conditions = match print_defrule_condition(store, node) {
        None => {return None;},
        Some(x) => inset(x),
    };
    let actions = match print_defrule_action(store, node) {
        None => {return None;},
        Some(x) => inset(x),
    };
    Some(format!("(defrule {} {}\n{}\n\t=>\n{}\n)",
        name, extras, conditions, actions))
}

fn print_defrule_condition(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    let list = match get_proplist(store, node, clips::conditional_element) {
        None => {return Some("".to_owned());},
        Some(x) => x,
    };
    let mut ret = String::new();
    for item in list {
        match print_conditional_element(store, item.as_ref()){
            None => {return None;},
            Some(x) => {ret.push_str(&x);},
        }
    }
    Some(ret)
}


fn print_defrule_action(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    let list = match get_proplist(store, node, clips::action) {
        None => {return Some("".to_owned());},
        Some(x) => x,
    };
    let mut ret = String::new();
    for item in list {
        match print_action(store, item.as_ref()){
            None => {return None;},
            Some(x) => {ret.push_str(&x);},
        }
    }
    Some(ret)
}


fn get_salience(store: &Store, node: NamedOrBlankNodeRef) -> Option<i32> {
    match query_literal(store, node, clips::salience.into()) {
        None => None,
        Some(lit) => {
            match i32::from_str_radix(lit.value(), 10){
                Ok(ret) => Some(ret),
                Err(_e) => None,
            }
        }
    }
}


fn print_lhs_slot(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let name = query_literal(store, node, clips::slot_name)?;
    let constraints = match query_obj(store, node, clips::constraint){
        Some(x) => vec![x],
        None => {
            match get_proplist(store, node, clips::constraints){
                None => {
                    return None;
                },
                Some(proplist) => proplist,
            }
        },
    };
    let mut args = String::new();
    for item in constraints {
        match print_constraint(store, item.as_ref()){
            None => {
                return None;
            },
            Some(x) => {
                args.push_str(" ");
                args.push_str(&x);
            },
        }
    }
    Some(format!("({} {})", name.value(), args))
}


fn print_assigned_pattern_ce(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    let var = query_obj(store, node, clips::fact_var_symbol)?;
    let var_print = print_variable(store, var.as_ref())?;
    let pattern = query_obj(store, node, clips::pattern_ce)?;
    let pattern_print = print_template_pattern_ce(store, pattern.as_ref())?;
    Some(format!("{} <- {}", var_print, pattern_print))
}


fn print_template_pattern_ce(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    let deftemplate = query_literal(store, node, clips::deftemplate_name)?;
    let qq = match get_proplist(store, node, clips::slot){
        None => vec![],
        Some(x) => x,
    };
    let mut tmp = String::new();
    for x in qq {
        tmp.push_str(" ");
        tmp.push_str(&print_lhs_slot(store, x.as_ref())?);
    }
    Some(format!("({} {})", deftemplate.value(), tmp))
}


fn print_conditional_element(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    if check_triple(store, node, rdf::TYPE, clips::AssignedPatternCE.into()){
        print_assigned_pattern_ce(store, node)
    } else if check_triple(store, node, rdf::TYPE, clips::TemplatePatternCE.into()){
        print_template_pattern_ce(store, node)
    } else if check_triple(store, node, rdf::TYPE, clips::TestCE.into()){
        print_test_ce(store, node)
    } else if check_triple(store, node, rdf::TYPE, clips::NotCE.into()){
        print_multi_ce(store, node)
    } else if check_triple(store, node, rdf::TYPE, clips::AndCE.into()) {
        print_multi_ce(store, node)
    } else if check_triple(store, node, rdf::TYPE, clips::ExistsCE.into()){
        print_multi_ce(store, node)
    } else if check_triple(store, node, rdf::TYPE, clips::OrCE.into()){
        print_multi_ce(store, node)
    } else {
        None
    }
}


fn print_multi_ce(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let cmd = if check_triple(store, node, rdf::TYPE, clips::NotCE.into()){
        "not"
    } else if check_triple(store, node, rdf::TYPE, clips::AndCE.into()) {
        "and"
    } else if check_triple(store, node, rdf::TYPE, clips::ExistsCE.into()){
        "exists"
    } else if check_triple(store, node, rdf::TYPE, clips::OrCE.into()){
        "or"
    } else {
        return None;
    };
    let mut tmp = String::new();
    let q = get_proplist(store, node, clips::conditional_element)?;
    for x in q {
        tmp.push_str(" ");
        tmp.push_str(&print_conditional_element(store, x.as_ref())?);
    }
    Some(format!("({} {})", cmd, tmp))
}


fn print_variable(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    match query_obj(store, node, clips::var_as_const_expr) {
        None => {},
        Some(x) => {return print_expression(store, x.as_ref());}
    }
    let varname = match query_literal(store, node, clips::variable_name) {
        None => {return None;},
        Some(x) => x,
    };
    if check_triple(store, node, rdf::TYPE, clips::MultifieldVariable.into()) {
        Some(format!("$?{}", varname.value()))
    } else {
        Some(format!("?{}", varname.value()))
    }
}


fn check_triple(store: &Store, subj: NamedOrBlankNodeRef, pred: NamedNodeRef, obj: TermRef) -> bool
{
    let q = store.quads_for_pattern(Some(subj), Some(pred), Some(obj), None);
    for x in q {
        match x {
            Ok(_y) => {return true;},
            Err(_e) => {return false;},
        }
    }
    false
}


fn print_variableslot(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let varnode = match query_obj(store, node, clips::member_variable){
        Some(x) => x,
        None => {return None;},
    };
    let varname = match query_literal(store, varnode.as_ref(), clips::variable_name){
        Some(x) => x,
        None => {return None;},
    };
    let slotname = match query_literal(store, node, clips::member_slot_name){
        Some(x) => x,
        None => {return None;},
    };
    Some(format!("?{}:{}", varname, slotname.value()))
}

fn print_expression(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    match print_constant(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_variable(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_variableslot(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_function(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    eprintln!("failed to print expression: {}\n", print_nodeinfo(store, node));
    //None
    Some("(fail)".to_owned())
}


fn print_action(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    print_expression(store, node)
}


fn print_deffacts(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let name = match query_literal(store, node, clips::deffacts_name) {
        Some(x) => x.value().to_owned(),
        None => format!("facts{}", Uuid::new_v4()),
    };
    let q = match get_proplist(store, node, clips::rhs_pattern){
        Some(x) => x,
        None => vec![],
    };
    let mut tmp = String::new();
    for item in q {
        tmp.push_str("\n");
        let x = print_template_rhs_pattern(store, item.as_ref())?;
        tmp.push_str(&inset(x));
    }
    Some(format!("(deffacts {} {}\n)", name, tmp))
}


fn print_constraint(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    if check_triple(store, node, rdf::TYPE, clips::SingleWildcard.into()){
        return Some("?".to_owned());
    } else if check_triple(store, node, rdf::TYPE, clips::MultiWildcard.into()){
        return Some("$?".to_owned());
    }
    match query_obj(store, node, clips::function_name) {
        Some(_) => {
            return Some(format!("= {}", print_function(store, node)?));
        },
        None =>{},
    }
    match query_obj(store, node, clips::var_as_const_expr){
        None => {},
        Some(_) => {
            return print_constraint(store, node);
        },
    }
    match print_constant(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_variable(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    eprintln!("failed to print constraint: {}\n", print_nodeinfo(store, node));
    //None
    Some("(fail)".to_owned())
}


fn print_term(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    match print_constant(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_variable(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_function(store, node){
        Some(x) => {return Some(x);},
        None => {},
    }
    eprintln!("failed to print term: {}\n", print_nodeinfo(store, node));
    //None
    Some("(fail)".to_owned())
}


fn print_constant(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    //Maybe just replace all of this with clips::lexeme or clips::value
    match query_literal(store, node, clips::symbol) {
        None => {},
        Some(x) => {
            return Some(format!("{}", x.value()));
        },
    }
    match query_literal(store, node, clips::string) {
        None => {},
        Some(x) => {
            //return Some(format!("\"{}\"", x.value()));
            return Some(format!("{}", x.value()));
        },
    }
    None
}


fn print_function(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    match query_obj(store, node, clips::assert) {
        None => {},
        Some(_x) => {return print_assert(store, node);}
    }
    match query_obj(store, node, clips::find_all_facts) {
        None => {},
        Some(_x) => {return print_find_facts(store, node);}
    }
    match query_obj(store, node, clips::find_fact) {
        None => {},
        Some(_x) => {return print_find_facts(store, node);}
    }
    match query_obj(store, node, clips::any_factp) {
        None => {},
        Some(_x) => {return print_find_facts(store, node);}
    }
    match query_obj(store, node, clips::do_for_all_facts) {
        None => {},
        Some(_x) => {return print_find_facts(store, node);}
    }
    match query_obj(store, node, clips::do_for_fact) {
        None => {},
        Some(_x) => {return print_find_facts(store, node);}
    }

    let mut ret = "(".to_owned();
    match query_obj(store, node, clips::function_name) {
        None => {return None;},
        Some(x) => {match print_term(store, x.as_ref()){
            None => {return None;},
            Some(y) => {ret.push_str(&y);},
        }},
    }
    match get_proplist(store, node, clips::function_args){
        None => {},
        Some(proplist) => {
            for item in proplist {
                ret.push_str(" ");
                match print_expression(store, item.as_ref()){
                    None => {return None;},
                    Some(x) => {ret.push_str(&x)},
                }
            }
        },
    }
    ret.push_str(")");
    Some(ret)
}


fn print_assert(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let assert = query_obj(store, node, clips::assert)?;
    let patternstring = print_template_rhs_pattern(store, assert.as_ref())?;
    Some(format!("(assert {})", patternstring))
}


fn print_find_facts(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    let cmd = if check_triple(store, node, rdf::TYPE, clips::find_all_facts.into()){
        "find-all-facts"
    } else if check_triple(store, node, rdf::TYPE, clips::find_fact.into()){
        "find-fact"
    } else if check_triple(store, node, rdf::TYPE, clips::any_factp.into()){
        "any-factp"
    } else if check_triple(store, node, rdf::TYPE, clips::do_for_all_facts.into()){
        "do-for-all-facts"
    } else if check_triple(store, node, rdf::TYPE, clips::do_for_fact.into()){
        "do-for-fact"
    } else {
        return None;
    };
    let qq = match get_proplist(store, node, clips::fact_set_template){
        Some(x) => x,
        None => vec![],
    };
    let mut fact_set_template = String::new();
    for item in qq {
        fact_set_template.push_str(" ");
        let x1 = query_literal(store, item.as_ref(), clips::fact_set_member_variable)?;
        let x2 = query_literal(store, item.as_ref(), clips::deftemplate_name)?;
        fact_set_template.push_str(&format!("({} {})", x1.value(), x2.value()));
    }
    let query = query_obj(store, node, clips::query)?;
    let querystring = print_function(store, query.as_ref())?;
    //if do for all facts:
    let qq2 = get_proplist(store, node, clips::action)?;
    let mut actionstring = String::new();
    for x in qq2 {
        actionstring.push_str(" ");
        actionstring.push_str(&print_function(store, x.as_ref())?);
    }
    Some(format!("({} {} {})", cmd, fact_set_template, actionstring))
}


fn print_template_rhs_pattern(store: &Store, node: NamedOrBlankNodeRef) -> Option<String>
{
    let name = query_literal(store, node, clips::deftemplate_name)?;
    let mut slots = String::new();
    let qq = get_proplist(store, node, clips::slot)?;
    for x in qq {
        slots.push_str(" ");
        slots.push_str(&print_rhs_slot(store, x.as_ref())?);
    }
    Some(format!("({} {})", name.value(), slots))
}


fn print_rhs_slot(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let name = query_literal(store, node, clips::slot_name)?;
    let mut field = String::new();
    let qq = get_proplist(store, node, clips::field)?;
    for x in qq {
        field.push_str(" ");
        field.push_str(&print_rhs_field(store, x.as_ref())?);
    }
    Some(format!("({} {})", name.value(), field))
}


fn print_rhs_field(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    match print_constant(store, node) {
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_variable(store, node) {
        Some(x) => {return Some(x);},
        None => {},
    }
    match print_function(store, node) {
        Some(x) => {return Some(x);},
        None => {},
    }
    eprintln!("failed to print rhs field: {}\n", print_nodeinfo(store, node));
    //None
    Some("(fail)".to_owned())
}


fn print_test_ce(store: &Store, node: NamedOrBlankNodeRef) -> Option<String> {
    let function = print_function(store, node)?;
    Some(format!("(test {})", function))
}

fn print_nodeinfo(store: &Store, node: NamedOrBlankNodeRef) -> String {
    let mut ret = format!("{}[", node);
    let q = store.quads_for_pattern(Some(node), None, None, None);
    for x in q {
        match x {
            Ok(y) => {ret.push_str(&format!("{} -> {}, ", y.predicate, y.object));},
            Err(_e) => {},
        }
    }
    ret.push_str("]");
    return ret;
}
