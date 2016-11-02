#include <assert.h>

#undef NDEBUG // FORCE ASSERT ACTIVATION

#include "term_variable.h"
#include "unify.h"

/*! Symbol key-word for a rewriting term. */
static char const *const symbol_rewrite = "rewrite";
/*! Symbol key-word for a single rewriting rule. */
static char const *const symbol_rule = "->";
/*! Symbol key-word for the result term. */
static char const *const symbol_results = "results";

/*! Symbol key-word for the affectation of variables. */
static char const *const symbol_affectation = "affectation";
/*! Symbol key-word for the valuation of one variable. */
static char const *const symbol_valuation = "valuation";

static term term_create_valuation(term variable, term value) {
  sstring valuation_sstring = sstring_create_string(symbol_valuation);
  term valuation = term_create(valuation_sstring);
  sstring_destroy(&valuation_sstring);
  term_add_argument_first(valuation, variable);
  term_add_argument_last(valuation, value);
  return valuation;
}

/*!
 * To check that a term correspond to a term pattern.
 * Terms \c t and \c pattern are not modified.
 * \param t term to check
 * \param pattern pattern to find
 * \param affectation term representing the variables set so far.
 * \pre t, pattern and affectation are non NULL.
 * \return true if \c pattern is matched. In such a case affectation is filled
 * accordingly.
 */
static bool term_is_pattern(term t, term pattern, term affectation) {
  if (term_is_variable(pattern)) {
    term newValuation = term_create_valuation(pattern, t);
    term_add_argument_last(affectation, newValuation);
    return true;
  }
  if (term_compare(t, pattern) == 0) {
    return true;
  } /*
     term_argument_traversal ttraversal = term_argument_traversal_create(t);
     term_argument_traversal patternTraversal =
         term_argument_traversal_create(pattern);
     while (term_argument_traversal_has_next(ttraversal)) {
       if (!term_argument_traversal_has_next(patternTraversal)) {
         return false;
       }
       term current_t = term_argument_traversal_get_next(ttraversal);
       term current_pattern =
   term_argument_traversal_get_next(patternTraversal);
       if (!term_is_pattern(current_t, current_pattern, affectation)) {
         return false;
       }
   }
     term_argument_traversal_destroy(&ttraversal);
     term_argument_traversal_destroy(&patternTraversal);*/
  if (term_compare(t, pattern) != 0) {
    return false;
  }
  return true;
}

/*!
 * Add an argument to a term in sorted position.
 * Sort is done according to term_compare.
 * If the added argument is already present, then it is not added.
 * \param t term to check.
 * \param arg term to add as argument.
 * \pre t and arg are non NULL.
 */
static void term_add_arg_sort_unique(term t, term arg) {
  int arity = term_get_arity(t);
  for (int i = 0; i < arity; i++) {
    if (term_compare(arg, term_get_argument(t, i)) == 0) {
      // destroy the term ?
      term_destroy(&arg);
      return;
    }
    if (term_compare(arg, term_get_argument(t, i)) < 0) {
      term_add_argument_position(t, arg, i);
      return;
    }
  }
  term_add_argument_last(t, arg);
}

term term_copy_replace_at_loc(term t, term r, term *loc) {
  term new = term_create(term_get_symbol(t));
  for (int i = 0; i < term_get_arity(t); i++) {
    term arg = term_get_argument(t, i);
    // term copyarg = term_copy(arg);
    if (*loc == arg) {
      term_add_argument_last(new, r);
    } else {
      term_add_argument_last(new, term_copy_replace_at_loc(arg, r, loc));
    }
  }
  return new;
}

/*!
 * To make operate a single rewriting rule on a term.
 * The products of rewriting are added to the results term as argument.
 * They are added sorted without duplicate.
 * Rewriting process is local, but the whole structure has to output.
 * \param t_whole whole term
 * \param t_current current sub-term being looked for a match
 * \param pattern pattern to match
 * \param replace term to replace matches
 * \param results terms already generated by previous rules and the current
 * rule.
 * \pre none of the term is NULL.
 */
static void term_rewrite_rule(term t_whole, term t_current, term pattern,
                              term replace, term results) {
  sstring string_affectation = sstring_create_string(symbol_affectation);
  term affectation = term_create(string_affectation);
  sstring_destroy(&string_affectation);
  if (term_is_pattern(t_current, pattern, affectation)) {
    term r = term_copy(replace);
    term_argument_traversal affectationTraversal =
        term_argument_traversal_create(affectation);
    while (term_argument_traversal_has_next(affectationTraversal)) {
      term variable = term_argument_traversal_get_next(affectationTraversal);
      term_replace_variable(r, term_get_symbol(variable),
                            term_get_argument(variable, 0));
    }
    // Je dois checker dans les arguments aussi
    term *loc = &t_current;
    term copy = term_copy_replace_at_loc(t_whole, r, loc);
    /*term new = term_create(term_get_symbol(t_whole));
    for (int i = 0; i < term_get_arity(t_whole); i++) {
      term arg = term_get_argument(t_whole, i);
      term copyarg = term_copy(arg);
      if (*loc == arg) {
        term_add_argument_last(new, r);
      } else {
        term_add_argument_last(new, copyarg);
      }
  }*/
    // term_destroy(loc);
    //*loc = r;
    term_add_arg_sort_unique(results, copy);
  } else {
    term_argument_traversal t_current_traversal =
        term_argument_traversal_create(t_current);
    while (term_argument_traversal_has_next(t_current_traversal)) {
      term_rewrite_rule(t_whole,
                        term_argument_traversal_get_next(t_current_traversal),
                        pattern, replace, results);
    }
    term_argument_traversal_destroy(&t_current_traversal);
  }
}

/*!
 * Check that rules are well formed.
 * It is supposed that:
 * \li any number first argument has been removed
 * \li last argument is the term to rewrite
 *
 * Should be used for assert only
 */
static bool rules_are_well_formed(term t) { return true; }

term term_rewrite(term t) {
  assert(rules_are_well_formed(t));
  // Here I suppose the rule is well formed
  int factor = 1;
  term firstArgument = term_get_argument(t, 0);
  if (!term_is_variable(firstArgument) && term_get_arity(firstArgument) == 0) {
    sstring_is_integer(term_get_symbol(firstArgument), &factor);
  }
  term termToRewrite = term_get_argument(t, term_get_arity(t) - 1);
  sstring string_result = sstring_create_string(symbol_results);
  term results = term_create(string_result);
  sstring_destroy(&string_result);
  term_argument_traversal rewriteTraversal = term_argument_traversal_create(t);

  if (factor > 1) {
    term_argument_traversal_get_next(rewriteTraversal);
  }

  while (term_argument_traversal_has_next(rewriteTraversal)) {
    term rule = term_argument_traversal_get_next(rewriteTraversal);
    if (!term_argument_traversal_has_next(rewriteTraversal)) {
      break;
    }
    term termToReplace = term_get_argument(rule, 0);
    term replaceWith = term_get_argument(rule, 1);
    term_argument_traversal args =
        term_argument_traversal_create(termToRewrite);
    while (term_argument_traversal_has_next(args)) {
      term_rewrite_rule(termToRewrite, term_argument_traversal_get_next(args),
                        termToReplace, replaceWith, results);
    }
    sstring string_result = sstring_create_string(symbol_results);
    term newResults = term_create(string_result);
    for (size_t i = 1; i < factor; i++) {
      term_argument_traversal argsToRewrite =
          term_argument_traversal_create(results);
      while (term_argument_traversal_has_next(argsToRewrite)) {
        term termToRewrite = term_argument_traversal_get_next(argsToRewrite);
        term_argument_traversal args =
            term_argument_traversal_create(termToRewrite);
        while (term_argument_traversal_has_next(args)) {
          term_rewrite_rule(termToRewrite,
                            term_argument_traversal_get_next(args),
                            termToReplace, replaceWith, newResults);
        }
      }
      // term_destroy(results);
      results = term_copy(newResults);
      // term_destroy(newResults);
      newResults = term_create(string_result);
    }
  }
  return results;
}
