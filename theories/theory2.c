/*
 * theory2.c
 * P vs NP — Attempt 2: First-Order Logic Expression
 *
 * Goal:
 *   Not to prove P ≠ NP — but to EXPRESS it precisely in FOL.
 *   We build the full machinery:
 *     - Vocabulary (sorts, constants, predicate symbols, function symbols)
 *     - Terms and atomic formulas
 *     - A structure M over which sentences are evaluated
 *     - A satisfaction relation M ⊨ φ
 *     - The FOL sentence that expresses P ≠ NP
 *     - Evaluation of that sentence in M
 *
 * The FOL sentence for P ≠ NP:
 *
 *   ∃L [ InNP(L) ∧ ¬InP(L) ]
 *
 * Expanded with full predicate definitions:
 *
 *   ∃L [ (∃M_v ∃k ∀x ∃c  Verifies(M_v, x, c, k)) ∧
 *        ¬(∃M_d ∃k ∀x    Decides(M_d, x, k))      ]
 *
 * Where Verifies and Decides are bounded by n^k steps.
 *
 * The Arithmetic Problem:
 *   "bounded by n^k steps" requires n^k to be computable inside
 *   the structure — this pulls in Peano Arithmetic (PA).
 *   We encode this honestly: the structure M carries an arithmetic
 *   component, and we flag exactly where pure FOL ends and
 *   arithmetic begins.
 *
 * What This File Builds:
 *   1. FOLTerm      — terms in the FOL language
 *   2. FOLFormula   — formulas (atomic, negation, conjunction,
 *                     disjunction, implication, quantified)
 *   3. Structure M  — domain + interpretation of predicates
 *   4. Valuation    — variable assignments
 *   5. satisfies()  — the M ⊨ φ[v] relation
 *   6. The sentence ∃L(InNP(L) ∧ ¬InP(L)) built and evaluated
 *   7. report_wall()— where FOL expression collapses
 *
 * Author: Swirly Crop
 * Series: P vs NP Deductive Attempts — #2
 * Builds on: deduce.c, theory1.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

/* ═══════════════════════════════════════════════════════════
 * SECTION 1 - FOL VOCABULARY
 *
 * Following the textbook (Ch. 14):
 *   Vocabulary = constant symbols + predicate symbols + function symbols
 *   We define these as enums so every symbol is explicit.
 * ═══════════════════════════════════════════════════════════ */

/* Sorts - the kinds of objects in our domain */
typedef enum {
    SORT_LANGUAGE,   /* a language L ⊆ Σ*                    */
    SORT_MACHINE,    /* a Turing machine M                    */
    SORT_NAT,        /* a natural number (for time bounds)    */
    SORT_STRING,     /* an input string x ∈ Σ*               */
    SORT_CERT,       /* a certificate c (witness for NP)      */
} Sort;

/* Predicate symbols - what we can say about objects */
typedef enum {
    PRED_IN_P,         /* InP(L)              : L ∈ P                  */
    PRED_IN_NP,        /* InNP(L)             : L ∈ NP                 */
    PRED_DECIDES,      /* Decides(M, L, x, k) : M decides x∈L in n^k  */
    PRED_VERIFIES,     /* Verifies(M,L,x,c,k) : M verifies c for x∈L  */
    PRED_POLY_TIME,    /* PolyTime(M, k)      : M runs in O(n^k)       */
    PRED_REDUCES_TO,   /* ReducesTo(L1,L2,f,k): poly reduction L1→L2  */
    PRED_MEMBER,       /* Member(x, L)        : x ∈ L                  */
    PRED_LEQ,          /* Leq(a, b)           : a ≤ b  (arithmetic)    */
    PRED_EQ,           /* Eq(a, b)            : a = b                  */
} PredSym;

/* Function symbols */
typedef enum {
    FUN_LENGTH,        /* |x|    : length of string x                  */
    FUN_POW,           /* n^k    : arithmetic power  (needs PA)        */
    FUN_COMPOSE,       /* f ∘ g  : function composition                */
} FunSym;

/* Constant symbols */
typedef enum {
    CONST_SAT,         /* the language SAT                             */
    CONST_3COLOR,      /* the language 3-colorability                  */
    CONST_HALT,        /* the Halting language                         */
    CONST_ZERO,        /* 0 ∈ ℕ                                        */
    CONST_ONE,         /* 1 ∈ ℕ                                        */
} ConstSym;

const char *sort_name(Sort s) {
    switch (s) {
        case SORT_LANGUAGE: return "Language";
        case SORT_MACHINE:  return "Machine";
        case SORT_NAT:      return "Nat";
        case SORT_STRING:   return "String";
        case SORT_CERT:     return "Cert";
        default:            return "Unknown";
    }
}

const char *pred_name(PredSym p) {
    switch (p) {
        case PRED_IN_P:       return "InP";
        case PRED_IN_NP:      return "InNP";
        case PRED_DECIDES:    return "Decides";
        case PRED_VERIFIES:   return "Verifies";
        case PRED_POLY_TIME:  return "PolyTime";
        case PRED_REDUCES_TO: return "ReducesTo";
        case PRED_MEMBER:     return "Member";
        case PRED_LEQ:        return "Leq";
        case PRED_EQ:         return "Eq";
        default:              return "Unknown";
    }
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 2 - TERMS AND FORMULAS
 *
 * Terms: variables, constants, function applications
 * Formulas: atomic, ¬φ, φ∧ψ, φ∨ψ, φ→ψ, ∀x.φ, ∃x.φ
 * ═══════════════════════════════════════════════════════════ */

typedef enum {
    TERM_VAR,     /* variable x, y, L, M, k, ...   */
    TERM_CONST,   /* constant symbol                */
    TERM_FUN,     /* function application f(t1,...) */
} TermKind;

typedef struct FOLTerm {
    TermKind   kind;
    char      *name;       /* variable name or constant label */
    Sort       sort;       /* what sort this term has         */
    FunSym     fun;        /* if TERM_FUN                     */
    struct FOLTerm **args; /* subterms                        */
    int        arity;      /* number of args                  */
} FOLTerm;

typedef enum {
    FORM_ATOM,    /* P(t1, ..., tn)  */
    FORM_NEG,     /* ¬φ              */
    FORM_AND,     /* φ ∧ ψ           */
    FORM_OR,      /* φ ∨ ψ           */
    FORM_IMP,     /* φ → ψ           */
    FORM_FORALL,  /* ∀x. φ           */
    FORM_EXISTS,  /* ∃x. φ           */
    FORM_BOTTOM,  /* ⊥               */
    FORM_TOP,     /* ⊤               */
} FormKind;

typedef struct FOLFormula {
    FormKind          kind;
    char             *label;      /* human-readable name for this formula */
    PredSym           pred;       /* if FORM_ATOM                         */
    FOLTerm         **args;       /* atom arguments                       */
    int               arity;      /* number of atom args                  */
    struct FOLFormula *left;      /* subformula (neg/and/or/imp/quant)    */
    struct FOLFormula *right;     /* right subformula (and/or/imp)        */
    char             *bound_var;  /* bound variable name (forall/exists)  */
    Sort              bound_sort; /* sort of bound variable               */
    /*
     * arithmetic_required: set to true when this subformula cannot be
     * evaluated in pure FOL — it requires arithmetic (n^k, |x|, etc.)
     * This flag is the honest tracker of where FOL ends.
     */
    bool              arithmetic_required;
} FOLFormula;

/* ─── constructors ─── */

FOLTerm *make_var(const char *name, Sort sort) {
    FOLTerm *t    = calloc(1, sizeof(FOLTerm));
    t->kind       = TERM_VAR;
    t->name       = strdup(name);
    t->sort       = sort;
    return t;
}

FOLTerm *make_const(ConstSym c, const char *name, Sort sort) {
    FOLTerm *t    = calloc(1, sizeof(FOLTerm));
    t->kind       = TERM_CONST;
    t->name       = strdup(name);
    t->sort       = sort;
    (void)c;
    return t;
}

FOLFormula *make_atom(PredSym pred, FOLTerm **args, int arity,
                       const char *label, bool needs_arith) {
    FOLFormula *f           = calloc(1, sizeof(FOLFormula));
    f->kind                 = FORM_ATOM;
    f->label                = strdup(label);
    f->pred                 = pred;
    f->args                 = args;
    f->arity                = arity;
    f->arithmetic_required  = needs_arith;
    return f;
}

FOLFormula *make_neg(FOLFormula *phi, const char *label) {
    FOLFormula *f           = calloc(1, sizeof(FOLFormula));
    f->kind                 = FORM_NEG;
    f->label                = strdup(label);
    f->left                 = phi;
    f->arithmetic_required  = phi->arithmetic_required;
    return f;
}

FOLFormula *make_and(FOLFormula *l, FOLFormula *r, const char *label) {
    FOLFormula *f           = calloc(1, sizeof(FOLFormula));
    f->kind                 = FORM_AND;
    f->label                = strdup(label);
    f->left                 = l;
    f->right                = r;
    f->arithmetic_required  = l->arithmetic_required || r->arithmetic_required;
    return f;
}

FOLFormula *make_exists(const char *var, Sort sort,
                         FOLFormula *body, const char *label) {
    FOLFormula *f           = calloc(1, sizeof(FOLFormula));
    f->kind                 = FORM_EXISTS;
    f->label                = strdup(label);
    f->bound_var            = strdup(var);
    f->bound_sort           = sort;
    f->left                 = body;
    f->arithmetic_required  = body->arithmetic_required;
    return f;
}

FOLFormula *make_forall(const char *var, Sort sort,
                         FOLFormula *body, const char *label) {
    FOLFormula *f           = calloc(1, sizeof(FOLFormula));
    f->kind                 = FORM_FORALL;
    f->label                = strdup(label);
    f->bound_var            = strdup(var);
    f->bound_sort           = sort;
    f->left                 = body;
    f->arithmetic_required  = body->arithmetic_required;
    return f;
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 3 — THE STRUCTURE M
 *
 * A structure M = (Domain, Interpretation) where:
 *   Domain    = the set of objects we quantify over
 *   Interp    = functions mapping predicate/function symbols
 *               to actual relations/functions on the domain
 *
 * Our domain has four sorts:
 *   Languages : represented as integer IDs (mock)
 *   Machines  : represented as function pointers + metadata
 *   Nats      : plain int (for time bounds)
 *   Strings   : char* (input strings)
 * ═══════════════════════════════════════════════════════════ */

/* A language in the domain — identified by ID and membership function */
typedef struct {
    int   id;
    char *name;
    bool  in_P;          /* ground truth for this mock structure    */
    bool  in_NP;         /* ground truth                            */
    bool (*member)(const char *x);  /* membership oracle            */
} DomainLanguage;

/* A machine in the domain */
typedef struct {
    int   id;
    char *name;
    int   poly_exponent;   /* runs in O(n^k) — k stored here        */
    bool (*run)(const char *x, const char *cert);  /* mock runner   */
} DomainMachine;

/* Mock language membership functions */
bool sat_member(const char *x)     { return strlen(x) > 2; }  /* mock */
bool color_member(const char *x)   { return strlen(x) % 2 == 0; }
bool halt_member(const char *x)    { (void)x; return false; } /* undecidable */

/* Mock machine runners */
bool poly_decider(const char *x, const char *cert) {
    (void)cert;
    return strlen(x) > 2;
}
bool np_verifier(const char *x, const char *cert) {
    return strlen(x) > 2 && cert && strlen(cert) > 0;
}

/* The structure itself */
typedef struct {
    /* Language domain */
    DomainLanguage  *languages;
    int              n_languages;
    /* Machine domain */
    DomainMachine   *machines;
    int              n_machines;
    /* Arithmetic component — this is where PA enters */
    bool             has_arithmetic;   /* true = PA included in M     */
} Structure;

/* Build our concrete structure M */
Structure build_structure(void) {
    Structure M;

    /* Languages */
    M.n_languages  = 3;
    M.languages    = calloc(3, sizeof(DomainLanguage));

    M.languages[0] = (DomainLanguage){
        0, "SAT", true, true, sat_member
    };
    M.languages[1] = (DomainLanguage){
        1, "3COLOR", false, true, color_member  /* assumed NP-complete */
    };
    M.languages[2] = (DomainLanguage){
        2, "HALT", false, false, halt_member    /* undecidable */
    };

    /* Machines */
    M.n_machines   = 2;
    M.machines     = calloc(2, sizeof(DomainMachine));

    M.machines[0]  = (DomainMachine){
        0, "poly_decider", 3, poly_decider
    };
    M.machines[1]  = (DomainMachine){
        1, "np_verifier", 2, np_verifier
    };

    /*
     * CRITICAL: arithmetic is required to interpret InP and InNP.
     * We set this flag and track it through satisfaction.
     */
    M.has_arithmetic = true;

    return M;
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 4 - VALUATION
 *
 * A valuation v assigns domain objects to free variables.
 * M ⊨ φ[v] means φ is satisfied in M under assignment v.
 * ═══════════════════════════════════════════════════════════ */

#define MAX_VARS 32

typedef struct {
    char  *name[MAX_VARS];
    Sort   sort[MAX_VARS];
    int    val[MAX_VARS];     /* index into domain for this sort */
    int    count;
} Valuation;

Valuation empty_valuation(void) {
    Valuation v;
    memset(&v, 0, sizeof(v));
    return v;
}

/* Extend valuation: bind var_name to domain index val */
Valuation extend(Valuation v, const char *var_name, Sort sort, int val) {
    if (v.count < MAX_VARS) {
        v.name[v.count]  = strdup(var_name);
        v.sort[v.count]  = sort;
        v.val[v.count]   = val;
        v.count++;
    }
    return v;
}

int lookup(Valuation *v, const char *name) {
    for (int i = v->count - 1; i >= 0; i--)
        if (strcmp(v->name[i], name) == 0)
            return v->val[i];
    return -1;  /* unbound */
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 5 - PREDICATE INTERPRETATIONS
 *
 * Each predicate symbol maps to a relation on the domain.
 * This is where the arithmetic enters honestly.
 * ═══════════════════════════════════════════════════════════ */

/*
 * interp_InP: L ∈ P
 *
 * Full definition:
 *   InP(L) ≡ ∃M ∃k ∀x [ Decides(M,L,x) ∧ Steps(M,x) ≤ |x|^k ]
 *
 * The |x|^k bound requires arithmetic — flagged below.
 * In our mock structure we use the ground-truth in_P field.
 */
bool interp_InP(Structure *M, int lang_idx, bool *needs_arith) {
    *needs_arith = true;   /* |x|^k requires PA */
    if (lang_idx < 0 || lang_idx >= M->n_languages) return false;
    printf("    [InP] Interpreting InP(%s): ground truth = %s\n",
           M->languages[lang_idx].name,
           M->languages[lang_idx].in_P ? "true" : "false");
    printf("    [InP] *** arithmetic_required: |x|^k bound needs PA ***\n");
    return M->languages[lang_idx].in_P;
}

/*
 * interp_InNP: L ∈ NP
 *
 * Full definition:
 *   InNP(L) ≡ ∃M_v ∃k ∀x [ (x∈L → ∃c [ |c|≤|x|^k ∧ Verifies(M_v,x,c) ])
 *                          ∧ (x∉L → ∀c ¬Verifies(M_v,x,c)) ]
 *
 * Again: |c| ≤ |x|^k requires arithmetic.
 */
bool interp_InNP(Structure *M, int lang_idx, bool *needs_arith) {
    *needs_arith = true;   /* certificate bound needs PA */
    if (lang_idx < 0 || lang_idx >= M->n_languages) return false;
    printf("    [InNP] Interpreting InNP(%s): ground truth = %s\n",
           M->languages[lang_idx].name,
           M->languages[lang_idx].in_NP ? "true" : "false");
    printf("    [InNP] *** arithmetic_required: |c|≤|x|^k bound needs PA ***\n");
    return M->languages[lang_idx].in_NP;
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 6 - THE SATISFACTION RELATION
 *
 * M ⊨ φ[v]
 *
 * Defined recursively on the structure of φ.
 * Returns a SatResult that tracks:
 *   - whether the formula is satisfied
 *   - whether arithmetic was required
 *   - a trace of the evaluation
 * ═══════════════════════════════════════════════════════════ */

typedef struct {
    bool satisfied;
    bool arithmetic_used;
    char reason[2048];
} SatResult;

/* Forward declaration */
SatResult satisfies(Structure *M, FOLFormula *phi, Valuation *v, int depth);

void indent(int depth) {
    for (int i = 0; i < depth * 2; i++) putchar(' ');
}

SatResult satisfies(Structure *M, FOLFormula *phi, Valuation *v, int depth) {
    SatResult result = { false, false, "" };

    switch (phi->kind) {

    case FORM_TOP:
        result.satisfied = true;
        snprintf(result.reason, sizeof(result.reason), "⊤");
        return result;

    case FORM_BOTTOM:
        result.satisfied = false;
        snprintf(result.reason, sizeof(result.reason), "⊥");
        return result;

    case FORM_ATOM: {
        indent(depth);
        printf("Atom: %s\n", phi->label);

        if (phi->pred == PRED_IN_P) {
            int L_idx = (phi->arity > 0 && phi->args[0]->kind == TERM_VAR)
                        ? lookup(v, phi->args[0]->name)
                        : 0;
            bool needs_arith = false;
            result.satisfied     = interp_InP(M, L_idx, &needs_arith);
            result.arithmetic_used = needs_arith;
            snprintf(result.reason, sizeof(result.reason),
                     "InP(%s) = %s [arith=%s]",
                     L_idx >= 0 ? M->languages[L_idx].name : "?",
                     result.satisfied ? "true" : "false",
                     needs_arith ? "YES" : "no");

        } else if (phi->pred == PRED_IN_NP) {
            int L_idx = (phi->arity > 0 && phi->args[0]->kind == TERM_VAR)
                        ? lookup(v, phi->args[0]->name)
                        : 0;
            bool needs_arith = false;
            result.satisfied     = interp_InNP(M, L_idx, &needs_arith);
            result.arithmetic_used = needs_arith;
            snprintf(result.reason, sizeof(result.reason),
                     "InNP(%s) = %s [arith=%s]",
                     L_idx >= 0 ? M->languages[L_idx].name : "?",
                     result.satisfied ? "true" : "false",
                     needs_arith ? "YES" : "no");

        } else {
            /* Other predicates — simplified */
            result.satisfied       = false;
            result.arithmetic_used = phi->arithmetic_required;
            snprintf(result.reason, sizeof(result.reason),
                     "%s: not fully interpreted (mock)", pred_name(phi->pred));
        }
        indent(depth);
        printf("  → %s\n", result.reason);
        return result;
    }

    case FORM_NEG: {
        indent(depth);
        printf("¬ %s\n", phi->left->label);
        SatResult sub = satisfies(M, phi->left, v, depth + 1);
        result.satisfied       = !sub.satisfied;
        result.arithmetic_used = sub.arithmetic_used;
        snprintf(result.reason, sizeof(result.reason),
                 "¬(%s)", sub.reason);
        indent(depth);
        printf("  → %s\n", result.reason);
        return result;
    }

    case FORM_AND: {
        indent(depth);
        printf("∧ [%s]\n", phi->label);
        SatResult l = satisfies(M, phi->left,  v, depth + 1);
        SatResult r = satisfies(M, phi->right, v, depth + 1);
        result.satisfied       = l.satisfied && r.satisfied;
        result.arithmetic_used = l.arithmetic_used || r.arithmetic_used;
        snprintf(result.reason, sizeof(result.reason),
                 "(%s) ∧ (%s)", l.reason, r.reason);
        indent(depth);
        printf("  → %s\n", result.reason);
        return result;
    }

    case FORM_EXISTS: {
        indent(depth);
        printf("∃%s:%s . %s\n",
               phi->bound_var, sort_name(phi->bound_sort), phi->left->label);

        /*
         * Existential: true if ANY domain element of the right sort
         * satisfies the body. We iterate over the domain.
         */
        int domain_size = (phi->bound_sort == SORT_LANGUAGE)
                          ? M->n_languages : M->n_machines;

        for (int i = 0; i < domain_size; i++) {
            Valuation v2 = extend(*v, phi->bound_var, phi->bound_sort, i);
            SatResult sub = satisfies(M, phi->left, &v2, depth + 1);
            result.arithmetic_used = result.arithmetic_used || sub.arithmetic_used;
            if (sub.satisfied) {
                result.satisfied = true;
                snprintf(result.reason, sizeof(result.reason),
                         "∃%s[%d]: %s", phi->bound_var, i, sub.reason);
                indent(depth);
                printf("  → ∃ witness at index %d: %s\n", i, result.reason);
                return result;
            }
        }

        result.satisfied = false;
        snprintf(result.reason, sizeof(result.reason),
                 "∃%s: no witness found in domain", phi->bound_var);
        indent(depth);
        printf("  → %s\n", result.reason);
        return result;
    }

    case FORM_FORALL: {
        indent(depth);
        printf("∀%s:%s . %s\n",
               phi->bound_var, sort_name(phi->bound_sort), phi->left->label);

        int domain_size = (phi->bound_sort == SORT_LANGUAGE)
                          ? M->n_languages : M->n_machines;

        result.satisfied = true;
        for (int i = 0; i < domain_size; i++) {
            Valuation v2 = extend(*v, phi->bound_var, phi->bound_sort, i);
            SatResult sub = satisfies(M, phi->left, &v2, depth + 1);
            result.arithmetic_used = result.arithmetic_used || sub.arithmetic_used;
            if (!sub.satisfied) {
                result.satisfied = false;
                snprintf(result.reason, sizeof(result.reason),
                         "∀%s fails at index %d: %s",
                         phi->bound_var, i, sub.reason);
                indent(depth);
                printf("  → ∀ fails at %d\n", i);
                return result;
            }
        }

        snprintf(result.reason, sizeof(result.reason),
                 "∀%s holds over domain", phi->bound_var);
        indent(depth);
        printf("  → ∀ holds\n");
        return result;
    }

    default:
        snprintf(result.reason, sizeof(result.reason), "unhandled form");
        return result;
    }
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 7 - BUILD THE P ≠ NP SENTENCE
 *
 * Target sentence:
 *   ∃L [ InNP(L) ∧ ¬InP(L) ]
 *
 * This says: there exists a language that is in NP but not in P.
 * That is precisely P ≠ NP (assuming P ⊆ NP, which is known).
 * ═══════════════════════════════════════════════════════════ */

FOLFormula *build_pneqnp_sentence(void) {
    printf("┌─ Building FOL sentence: ∃L [ InNP(L) ∧ ¬InP(L) ]\n\n");

    /* Variable L of sort Language */
    FOLTerm *var_L = make_var("L", SORT_LANGUAGE);

    FOLTerm **args_np = malloc(sizeof(FOLTerm *));
    args_np[0] = var_L;
    FOLTerm **args_p  = malloc(sizeof(FOLTerm *));
    args_p[0]  = var_L;

    /* Atomic formulas */
    /* InNP(L) — arithmetic required for certificate bound */
    FOLFormula *in_np = make_atom(PRED_IN_NP, args_np, 1,
                                   "InNP(L)", true);

    /* InP(L) — arithmetic required for runtime bound */
    FOLFormula *in_p  = make_atom(PRED_IN_P,  args_p,  1,
                                   "InP(L)",  true);

    /* ¬InP(L) */
    FOLFormula *not_p = make_neg(in_p, "¬InP(L)");

    /* InNP(L) ∧ ¬InP(L) */
    FOLFormula *body  = make_and(in_np, not_p, "InNP(L) ∧ ¬InP(L)");

    /* ∃L . InNP(L) ∧ ¬InP(L) */
    FOLFormula *sentence = make_exists("L", SORT_LANGUAGE, body,
                                        "∃L[InNP(L) ∧ ¬InP(L)]");

    printf("  Vocabulary used:\n");
    printf("    Predicate: InNP  (arity 1, sort: Language)\n");
    printf("    Predicate: InP   (arity 1, sort: Language)\n");
    printf("    Variable:  L     (sort: Language)\n");
    printf("    Connectives: ∧, ¬\n");
    printf("    Quantifier:  ∃\n\n");

    printf("  Arithmetic flags:\n");
    printf("    InNP(L): arithmetic_required = true  (|c|≤|x|^k)\n");
    printf("    InP(L):  arithmetic_required = true  (steps≤|x|^k)\n");
    printf("    Full sentence: arithmetic_required = %s\n\n",
           sentence->arithmetic_required ? "TRUE" : "false");

    printf("└─ Sentence built\n\n");
    return sentence;
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 8 - EVALUATE THE SENTENCE IN M
 * ═══════════════════════════════════════════════════════════ */

void evaluate_sentence(Structure *M, FOLFormula *phi) {
    printf("┌─ Evaluating: M ⊨ %s ?\n\n", phi->label);

    Valuation v = empty_valuation();
    SatResult result = satisfies(M, phi, &v, 1);

    printf("\n└─ Result: M ⊨ %s is %s\n",
           phi->label,
           result.satisfied ? "TRUE" : "FALSE");
    printf("   Arithmetic used: %s\n",
           result.arithmetic_used ? "YES — PA required" : "no");
    printf("   Reason: %s\n\n", result.reason);
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 9 - PRINT THE FULL SENTENCE STRUCTURE
 * ═══════════════════════════════════════════════════════════ */

void print_fol_sentence(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║         FOL EXPRESSION OF P ≠ NP                    ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    printf("Surface form:\n\n");
    printf("  ∃L [ InNP(L) ∧ ¬InP(L) ]\n\n");

    printf("Expanded (InP unpacked):\n\n");
    printf("  ∃L [\n");
    printf("    ( ∃M_v ∃k ∀x ∃c\n");
    printf("        Verifies(M_v, x, c) ∧ Steps(M_v,x,c) ≤ |x|^k )\n");
    printf("    ∧\n");
    printf("    ¬( ∃M_d ∃k ∀x\n");
    printf("        Decides(M_d, x) ∧ Steps(M_d,x) ≤ |x|^k )\n");
    printf("  ]\n\n");

    printf("Quantifier structure (depth):\n\n");
    printf("  ∃L          — language quantifier          [sort: Language]\n");
    printf("  ├ ∃M_v      — verifier machine             [sort: Machine ]\n");
    printf("  ├ ∃k        — polynomial exponent          [sort: Nat     ]\n");
    printf("  ├ ∀x        — universal input              [sort: String  ]\n");
    printf("  ├ ∃c        — certificate witness          [sort: Cert    ]\n");
    printf("  ├ ∃M_d      — decider machine (negated)    [sort: Machine ]\n");
    printf("  └ ∃k'       — polynomial exponent (negated)[sort: Nat     ]\n\n");

    printf("Arithmetic subterms (require PA):\n\n");
    printf("  |x|^k  — length of x to the k-th power\n");
    printf("  Steps(M,x) ≤ |x|^k — step count bounded by polynomial\n\n");

    printf("FOL boundary:\n\n");
    printf("  Pure FOL  : ∃L, ∃M, ∃k, ∀x, ∃c — all first-order\n");
    printf("  Requires PA: |x|^k, Steps(M,x) ≤ n^k\n");
    printf("  The sentence is FOL *relative to a structure with arithmetic*\n\n");
}

/* ═══════════════════════════════════════════════════════════
 * SECTION 10 — WHERE THIS ATTEMPT STANDS
 * ═══════════════════════════════════════════════════════════ */

void report_wall(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║         WHERE ATTEMPT 2 STANDS                      ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    printf("ACHIEVEMENT:\n");
    printf("  The sentence ∃L[InNP(L) ∧ ¬InP(L)] is a well-formed\n");
    printf("  FOL sentence over our vocabulary. It expresses P ≠ NP\n");
    printf("  correctly. The satisfaction relation M ⊨ φ is defined\n");
    printf("  and evaluated. This is genuine progress over Attempt 1.\n\n");

    printf("THE WALL — THREE LAYERS:\n\n");

    printf("  Layer 1: Arithmetic\n");
    printf("    InP and InNP cannot be expressed in pure FOL without\n");
    printf("    importing arithmetic for the |x|^k time bound.\n");
    printf("    We need PA or at minimum bounded arithmetic (S¹₂).\n");
    printf("    The sentence lives in FOL+PA, not pure FOL.\n\n");

    printf("  Layer 2: The Model Problem\n");
    printf("    Gödel completeness: ⊢ φ iff ⊨ φ (valid in all models).\n");
    printf("    P ≠ NP is NOT valid in all models of our axioms.\n");
    printf("    In oracle models (Baker-Gill-Solovay), P = NP holds.\n");
    printf("    So ∃L[InNP(L) ∧ ¬InP(L)] is not a FOL tautology.\n");
    printf("    FOL cannot prove it from standard complexity axioms.\n\n");

    printf("  Layer 3: Expressiveness ceiling\n");
    printf("    Even in FOL+PA, P ≠ NP may be independent of the\n");
    printf("    axioms (analogous to CH independence from ZFC).\n");
    printf("    Proving it would require new axioms or a new method.\n\n");

    printf("WHAT WAS GAINED:\n");
    printf("  - A precise FOL vocabulary for complexity theory\n");
    printf("  - A formally defined structure M with satisfaction\n");
    printf("  - The exact boundary between FOL and arithmetic\n");
    printf("  - Confirmation that the sentence is syntactically correct\n");
    printf("  - Clear identification of the model-theoretic obstacle\n\n");

    printf("NEXT — Attempt 3:\n");
    printf("  Move into bounded arithmetic S¹₂ (Buss 1986).\n");
    printf("  This is a formal system specifically calibrated to\n");
    printf("  polynomial time — it can express InP and InNP without\n");
    printf("  full PA, and its proof complexity directly connects\n");
    printf("  to P vs NP. If P ≠ NP then certain tautologies have\n");
    printf("  no short S¹₂ proofs. We will encode that structure.\n\n");
}

/* ═══════════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════════ */

int main(void) {
    printf("theory2.c — P vs NP Attempt 2: FOL Expression\n");
    printf("Swirly Crop\n");
    printf("Builds on: deduce.c, theory1.c\n\n");

    /* Print the full sentence */
    print_fol_sentence();

    /* Build the structure */
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║         BUILDING STRUCTURE M                        ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");
    Structure M = build_structure();
    printf("  Domain:\n");
    printf("    Languages: %d  (SAT, 3COLOR, HALT)\n", M.n_languages);
    printf("    Machines:  %d  (poly_decider, np_verifier)\n", M.n_machines);
    printf("    Arithmetic component: %s\n\n",
           M.has_arithmetic ? "present (PA)" : "absent");

    /* Build and evaluate the sentence */
    FOLFormula *phi = build_pneqnp_sentence();
    evaluate_sentence(&M, phi);

    /* Report */
    report_wall();

    /* cleanup */
    free(M.languages);
    free(M.machines);

    return 0;
}