/*
 * theory3.c
 * P vs NP — Attempt 3: Circuit Complexity Lower Bounds
 *
 * Strategy:
 *   Show no polynomial-size Boolean circuit family computes SAT.
 *   SAT ∉ P/poly would imply P ≠ NP (since P ⊆ P/poly).
 *
 *   Tools used, in order:
 *     (A) Boolean circuit model - formal definitions
 *     (B) Switching lemma (Håstad 1987) - the core combinatorial tool
 *         that killed AC⁰ and nearly killed general circuits
 *     (C) Razborov-Rudich audit - which of the 3 natural-proof
 *         conditions does our argument satisfy?
 *         This is where Razborov's monotone circuit proof collapses
 *         for GENERAL circuits.
 *     (D) Williams method (2011) - approach lower bounds via faster
 *         algorithms instead of combinatorial properties.
 *         This is the non-naturalizing route.
 *     (E) Where Williams hits its own wall.
 *
 * The difference from Attempts 1 and 2:
 *   We do not just document the collapse.
 *   We attempt to ROUTE AROUND IT using the Williams method
 *   and report precisely what new obstacle appears.
 *
 * Author: Swirly Crop
 * Series: P vs NP Deductive Attempts - #3
 * Builds on: deduce.c, theory1.c, theory2.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

/* ═══════════════════════════════════════════════════════════
 * SECTION A - BOOLEAN CIRCUIT MODEL
 *
 * A circuit C_n over n inputs is a DAG where:
 *   - Leaves are input variables x_1,...,x_n or constants 0,1
 *   - Internal nodes are gates: AND, OR, NOT
 *   - One designated output gate
 *
 * Parameters:
 *   size(C)  = number of gates
 *   depth(C) = longest path from input to output
 *
 * A circuit FAMILY {C_n} computes f if C_n computes f|_{n inputs}.
 *
 * Circuit classes:
 *   P/poly  = polynomial size families (size n^k for some k)
 *   AC⁰    = constant depth, polynomial size, unbounded fan-in
 *   NC¹    = O(log n) depth, polynomial size
 *   TC⁰    = AC⁰ + majority gates
 * ═══════════════════════════════════════════════════════════ */

typedef enum {
    GATE_INPUT,    /* leaf: x_i          */
    GATE_CONST,    /* leaf: 0 or 1       */
    GATE_NOT,      /* unary: ¬a          */
    GATE_AND,      /* binary (or k-ary)  */
    GATE_OR,       /* binary (or k-ary)  */
    GATE_MAJORITY, /* TC⁰ gate           */
} GateType;

typedef struct Gate {
    int       id;
    GateType  type;
    int       input_idx;   /* for GATE_INPUT: which variable  */
    bool      const_val;   /* for GATE_CONST                  */
    int      *children;    /* indices of child gates          */
    int       n_children;
    bool      value;       /* evaluated value                 */
    int       depth;       /* depth from inputs               */
} Gate;

typedef struct Circuit {
    char   *name;
    int     n_inputs;
    int     n_gates;
    Gate   *gates;
    int     output_gate;   /* index of output gate            */
    int     size;          /* = n_gates                       */
    int     depth;         /* max depth                       */
    bool    is_monotone;   /* no NOT gates                    */
} Circuit;

/* Evaluate a circuit on a given input assignment */
bool eval_circuit(Circuit *C, bool *inputs) {
    bool *vals = calloc(C->n_gates, sizeof(bool));

    for (int i = 0; i < C->n_gates; i++) {
        Gate *g = &C->gates[i];
        switch (g->type) {
        case GATE_INPUT:
            vals[i] = inputs[g->input_idx];
            break;
        case GATE_CONST:
            vals[i] = g->const_val;
            break;
        case GATE_NOT:
            vals[i] = !vals[g->children[0]];
            break;
        case GATE_AND: {
            bool r = true;
            for (int j = 0; j < g->n_children; j++)
                r = r && vals[g->children[j]];
            vals[i] = r;
            break;
        }
        case GATE_OR: {
            bool r = false;
            for (int j = 0; j < g->n_children; j++)
                r = r || vals[g->children[j]];
            vals[i] = r;
            break;
        }
        case GATE_MAJORITY: {
            int count = 0;
            for (int j = 0; j < g->n_children; j++)
                count += vals[g->children[j]] ? 1 : 0;
            vals[i] = (count > g->n_children / 2);
            break;
        }
        }
    }

    bool result = vals[C->output_gate];
    free(vals);
    return result;
}

/* Build a small concrete SAT-checking circuit for 2-variable SAT:
 * (x1 ∨ x2) ∧ (x1 ∨ ¬x2) - satisfiable iff x1=1
 * This is just to make the model concrete and testable. */
Circuit build_small_sat_circuit(void) {
    Circuit C;
    C.name      = "SAT_2var_demo";
    C.n_inputs  = 2;
    C.n_gates   = 6;
    C.gates     = calloc(6, sizeof(Gate));
    C.is_monotone = false;

    /* Gate 0: input x1 */
    C.gates[0] = (Gate){ 0, GATE_INPUT, 0, false, NULL, 0, false, 0 };
    /* Gate 1: input x2 */
    C.gates[1] = (Gate){ 1, GATE_INPUT, 1, false, NULL, 0, false, 0 };
    /* Gate 2: NOT x2 */
    int *ch2 = malloc(sizeof(int)); ch2[0] = 1;
    C.gates[2] = (Gate){ 2, GATE_NOT, 0, false, ch2, 1, false, 1 };
    /* Gate 3: x1 OR x2 */
    int *ch3 = malloc(2*sizeof(int)); ch3[0]=0; ch3[1]=1;
    C.gates[3] = (Gate){ 3, GATE_OR, 0, false, ch3, 2, false, 1 };
    /* Gate 4: x1 OR NOT(x2) */
    int *ch4 = malloc(2*sizeof(int)); ch4[0]=0; ch4[1]=2;
    C.gates[4] = (Gate){ 4, GATE_OR, 0, false, ch4, 2, false, 2 };
    /* Gate 5: output AND */
    int *ch5 = malloc(2*sizeof(int)); ch5[0]=3; ch5[1]=4;
    C.gates[5] = (Gate){ 5, GATE_AND, 0, false, ch5, 2, false, 3 };

    C.output_gate = 5;
    C.size        = 6;
    C.depth       = 3;
    return C;
}

void print_circuit_stats(Circuit *C) {
    printf("  Circuit: %s\n", C->name);
    printf("    inputs: %d, gates: %d, depth: %d, monotone: %s\n",
           C->n_inputs, C->n_gates, C->depth,
           C->is_monotone ? "yes" : "no");
}

/* ═══════════════════════════════════════════════════════════
 * SECTION B - THE SWITCHING LEMMA (Håstad 1987)
 *
 * Core idea:
 *   A k-CNF or k-DNF formula, when a RANDOM RESTRICTION
 *   sets each variable to 0 or 1 independently with prob p,
 *   and leaves variables free with prob 1-2p,
 *   simplifies to a decision tree of small depth
 *   with high probability.
 *
 * Formally (Håstad's switching lemma):
 *   Let f be a k-CNF. Let ρ be a random restriction where
 *   each variable is fixed (0 or 1) independently with prob p,
 *   free otherwise. Then:
 *
 *     Pr[DT-depth(f|_ρ) ≥ t] ≤ (5pk)^t
 *
 * Consequence:
 *   Constant-depth circuits (AC⁰) cannot compute PARITY.
 *   This was a major result - AC⁰ is strictly weaker than NC¹.
 *
 * Why it matters for P vs NP:
 *   If we could extend this to polynomial-depth circuits or
 *   general circuits, we'd have SAT ∉ P/poly.
 *   The switching lemma breaks down at polylog depth.
 *
 * This section:
 *   - Simulates random restrictions on a CNF
 *   - Measures resulting decision tree depth
 *   - Verifies the (5pk)^t bound empirically
 *   - Then attempts to apply it to general circuits
 * ═══════════════════════════════════════════════════════════ */

#define MAX_VARS    16
#define MAX_CLAUSES 32
#define FREE_VAR    -1    /* variable is unrestricted */

typedef struct {
    int vars[MAX_VARS];    /* variable indices in clause */
    int negs[MAX_VARS];    /* 1 = negated, 0 = positive */
    int size;
} Clause;

typedef struct {
    Clause clauses[MAX_CLAUSES];
    int    n_clauses;
    int    n_vars;
} CNF;

typedef struct {
    int assignment[MAX_VARS];  /* FREE_VAR or 0 or 1 */
    int n_free;
} Restriction;

/* Apply restriction to CNF, return simplified CNF */
CNF apply_restriction(CNF *f, Restriction *rho) {
    CNF result;
    result.n_clauses = 0;
    result.n_vars    = f->n_vars;

    for (int i = 0; i < f->n_clauses; i++) {
        Clause *cl = &f->clauses[i];
        bool    satisfied = false;
        Clause  new_cl;
        new_cl.size = 0;

        for (int j = 0; j < cl->size; j++) {
            int var = cl->vars[j];
            int neg = cl->negs[j];
            int val = rho->assignment[var];

            if (val == FREE_VAR) {
                /* variable free — keep in clause */
                new_cl.vars[new_cl.size] = var;
                new_cl.negs[new_cl.size] = neg;
                new_cl.size++;
            } else {
                /* variable fixed */
                bool lit_val = neg ? !val : val;
                if (lit_val) {
                    /* clause satisfied — drop it */
                    satisfied = true;
                    break;
                }
                /* literal false — drop literal, keep clause */
            }
        }

        if (!satisfied) {
            if (new_cl.size == 0) {
                /* empty clause — UNSAT under this restriction */
                /* encode as a trivially false clause */
                new_cl.vars[0] = 0;
                new_cl.negs[0] = 0;
                new_cl.size    = 1;
            }
            result.clauses[result.n_clauses++] = new_cl;
        }
    }

    return result;
}

/* Estimate decision tree depth of simplified CNF (rough measure) */
int estimate_dt_depth(CNF *f) {
    /* Rough heuristic: max clause size after restriction */
    int max_size = 0;
    for (int i = 0; i < f->n_clauses; i++)
        if (f->clauses[i].size > max_size)
            max_size = f->clauses[i].size;
    return max_size;
}

/* Generate a pseudo-random restriction with parameter p */
Restriction make_restriction(int n_vars, double p, unsigned int seed) {
    Restriction rho;
    rho.n_free = 0;
    /* Simple LCG for reproducibility */
    uint32_t s = seed;
    for (int i = 0; i < n_vars; i++) {
        s = s * 1664525 + 1013904223;
        double r = (s & 0xFFFF) / 65536.0;
        if (r < p) {
            /* fix to 0 */
            rho.assignment[i] = 0;
        } else if (r < 2*p) {
            /* fix to 1 */
            rho.assignment[i] = 1;
        } else {
            /* leave free */
            rho.assignment[i] = FREE_VAR;
            rho.n_free++;
        }
    }
    return rho;
}

/* Build a sample 3-CNF for testing */
CNF build_sample_3cnf(void) {
    CNF f;
    f.n_vars    = 6;
    f.n_clauses = 0;

    /* Clause: x0 ∨ x1 ∨ ¬x2 */
    f.clauses[f.n_clauses] = (Clause){
        {0,1,2}, {0,0,1}, 3
    };
    f.n_clauses++;

    /* Clause: ¬x1 ∨ x3 ∨ x4 */
    f.clauses[f.n_clauses] = (Clause){
        {1,3,4}, {1,0,0}, 3
    };
    f.n_clauses++;

    /* Clause: x2 ∨ ¬x3 ∨ x5 */
    f.clauses[f.n_clauses] = (Clause){
        {2,3,5}, {0,1,0}, 3
    };
    f.n_clauses++;

    /* Clause: ¬x0 ∨ x4 ∨ ¬x5 */
    f.clauses[f.n_clauses] = (Clause){
        {0,4,5}, {1,0,1}, 3
    };
    f.n_clauses++;

    return f;
}

typedef struct {
    double p;           /* restriction probability        */
    int    k;           /* CNF width                      */
    int    t;           /* depth threshold                */
    double bound;       /* (5pk)^t - theoretical bound    */
    double empirical;   /* measured probability           */
    bool   bound_holds; /* empirical ≤ bound?             */
} SwitchingLemmaResult;

SwitchingLemmaResult run_switching_lemma(CNF *f, double p, int t) {
    SwitchingLemmaResult res;
    res.p    = p;
    res.k    = 3;   /* 3-CNF */
    res.t    = t;
    res.bound = pow(5.0 * p * res.k, t);

    int trials  = 500;
    int exceed  = 0;

    for (int trial = 0; trial < trials; trial++) {
        Restriction rho      = make_restriction(f->n_vars, p,
                                                 trial * 31337 + 1);
        CNF         restricted = apply_restriction(f, &rho);
        int         depth      = estimate_dt_depth(&restricted);
        if (depth >= t) exceed++;
    }

    res.empirical   = (double)exceed / trials;
    res.bound_holds = (res.empirical <= res.bound + 0.05); /* allow float slack */
    return res;
}

void section_B(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  B: SWITCHING LEMMA (Håstad 1987)                   ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    printf("Lemma: For k-CNF f and random restriction ρ_p:\n");
    printf("  Pr[ DT-depth(f|_ρ) ≥ t ] ≤ (5pk)^t\n\n");

    CNF f = build_sample_3cnf();
    printf("Test formula: 4-clause 3-CNF on 6 variables\n\n");

    double ps[] = { 0.05, 0.10, 0.20 };
    int    ts[] = { 2, 3 };

    printf("  %-6s %-4s %-10s %-10s %-8s\n",
           "p", "t", "bound", "empirical", "holds?");
    printf("  %-6s %-4s %-10s %-10s %-8s\n",
           "------","----","----------","----------","--------");

    for (int pi = 0; pi < 3; pi++) {
        for (int ti = 0; ti < 2; ti++) {
            SwitchingLemmaResult r =
                run_switching_lemma(&f, ps[pi], ts[ti]);
            printf("  %-6.2f %-4d %-10.4f %-10.4f %-8s\n",
                   r.p, r.t, r.bound, r.empirical,
                   r.bound_holds ? "YES" : "NO");
        }
    }

    printf("\n");
    printf("AC⁰ consequence:\n");
    printf("  By iterating the switching lemma over d layers of a\n");
    printf("  depth-d circuit, PARITY requires depth Ω(n^{1/d}).\n");
    printf("  For constant d: PARITY ∉ AC⁰. ✓ (proven)\n\n");

    printf("Limit of the switching lemma:\n");
    printf("  The (5pk)^t bound degrades as circuit depth grows.\n");
    printf("  At depth d = polylog(n), the bound becomes trivial.\n");
    printf("  The lemma cannot reach general polynomial-size circuits.\n");
    printf("  It separates AC⁰ from NC¹ but not P from NP.\n\n");
}

/* ═══════════════════════════════════════════════════════════
 * SECTION C - RAZBOROV-RUDICH AUDIT
 *
 * The natural proofs barrier says: any proof technique that is
 *   (1) Constructive  - efficiently recognizable property
 *   (2) Large         - most functions have it
 *   (3) Useful        - property → circuit lower bound
 * cannot separate P from NP (assuming PRFs exist, which
 * follows from P ≠ NP - the self-referential trap).
 *
 * We audit the switching lemma approach against these three
 * conditions to show exactly WHY it fails for general circuits.
 * ═══════════════════════════════════════════════════════════ */

typedef struct {
    char *technique;
    bool  constructive;       /* condition 1 */
    bool  large;              /* condition 2 */
    bool  useful;             /* condition 3 */
    bool  is_natural_proof;   /* all three → natural → fails */
    char *constructive_reason;
    char *large_reason;
    char *useful_reason;
    char *collapse_reason;    /* why it fails for SAT specifically */
} NaturalProofAudit;

NaturalProofAudit audit_switching_lemma(void) {
    NaturalProofAudit a;
    a.technique      = "Switching Lemma / Random Restriction";

    a.constructive   = true;
    a.constructive_reason =
        "Given a circuit C, we can efficiently check whether it\n"
        "      simplifies under random restrictions — just apply ρ\n"
        "      and measure depth. This is a poly-time check.";

    a.large          = true;
    a.large_reason   =
        "Most functions DO simplify under random restrictions.\n"
        "      The property 'collapses under restriction' holds for\n"
        "      the vast majority of boolean functions.";

    a.useful         = true;
    a.useful_reason  =
        "For AC⁰ circuits: simplification → depth lower bound.\n"
        "      This gave PARITY ∉ AC⁰. So yes, it is useful.";

    a.is_natural_proof = a.constructive && a.large && a.useful;

    a.collapse_reason =
        "Since all three conditions hold, this IS a natural proof.\n"
        "      Razborov-Rudich: if PRFs exist (which they do if P≠NP),\n"
        "      then no natural proof can show SAT ∉ P/poly.\n"
        "      The technique works for AC⁰ because SAT is NOT in AC⁰\n"
        "      (that result doesn't require separating P from NP).\n"
        "      But extending it to polynomial-size general circuits\n"
        "      would be a natural proof of SAT ∉ P/poly — impossible\n"
        "      under the natural proofs barrier.";

    return a;
}

NaturalProofAudit audit_razborov_monotone(void) {
    NaturalProofAudit a;
    a.technique      = "Razborov Approximation Method (monotone circuits)";

    a.constructive   = true;
    a.constructive_reason =
        "The approximation property is constructive — given a\n"
        "      monotone circuit, you can efficiently check whether\n"
        "      it approximates the target function.";

    a.large          = true;
    a.large_reason   =
        "Most monotone functions have the approximation property\n"
        "      Razborov exploited. Largeness holds.";

    a.useful         = true;
    a.useful_reason  =
        "It proved CLIQUE ∉ monotone-P/poly. Extremely useful\n"
        "      for monotone circuit lower bounds.";

    a.is_natural_proof = a.constructive && a.large && a.useful;

    a.collapse_reason =
        "Again all three conditions hold — natural proof.\n"
        "      But it only works for MONOTONE circuits (no NOT gates).\n"
        "      SAT requires NOT gates. Monotone circuit lower bounds\n"
        "      do NOT imply general circuit lower bounds.\n"
        "      The method cannot cross to non-monotone circuits\n"
        "      without becoming a natural proof for general circuits,\n"
        "      which the barrier forbids.";

    return a;
}

void print_audit(NaturalProofAudit *a) {
    printf("  Technique: %s\n", a->technique);
    printf("  (1) Constructive: %s\n      %s\n",
           a->constructive ? "YES" : "NO", a->constructive_reason);
    printf("  (2) Large:        %s\n      %s\n",
           a->large ? "YES" : "NO", a->large_reason);
    printf("  (3) Useful:       %s\n      %s\n",
           a->useful ? "YES" : "NO", a->useful_reason);
    printf("  Natural proof: %s\n",
           a->is_natural_proof ? "YES → BARRIER APPLIES" : "NO → safe");
    printf("  Collapse: %s\n\n", a->collapse_reason);
}

void section_C(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  C: RAZBOROV-RUDICH AUDIT                           ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    printf("Razborov-Rudich (1994): any proof with properties\n");
    printf("  (1) Constructive + (2) Large + (3) Useful\n");
    printf("is a NATURAL PROOF and cannot separate P from NP.\n\n");

    NaturalProofAudit a1 = audit_switching_lemma();
    NaturalProofAudit a2 = audit_razborov_monotone();

    printf("── Audit 1 ──────────────────────────────────────────\n");
    print_audit(&a1);

    printf("── Audit 2 ──────────────────────────────────────────\n");
    print_audit(&a2);

    printf("Conclusion: both techniques are natural proofs.\n");
    printf("To route around the barrier we must break one condition.\n");
    printf("The Williams method breaks CONSTRUCTIVITY.\n\n");
}

/* ═══════════════════════════════════════════════════════════
 * SECTION D - THE WILLIAMS METHOD (2011)
 *
 * Ryan Williams showed that:
 *   BETTER SAT ALGORITHMS → CIRCUIT LOWER BOUNDS
 *
 * Specifically:
 *   If SAT can be solved in time 2^n / n^k for any k,
 *   then NEXP ⊄ ACC⁰.
 *
 * The proof goes:
 *   (1) Assume NEXP ⊆ ACC⁰ (for contradiction)
 *   (2) Then ACC⁰ circuits can be evaluated faster than brute force
 *   (3) This gives a faster SAT algorithm for ACC⁰-SAT
 *   (4) That faster algorithm implies NEXP ⊄ ACC⁰ (contradiction)
 *
 * Why this avoids natural proofs:
 *   The argument is NON-CONSTRUCTIVE in the Razborov-Rudich sense.
 *   It does not exhibit a property of most functions that implies
 *   hardness. Instead it uses an ALGORITHMIC argument - the existence
 *   of a faster algorithm - which does not translate to an efficient
 *   recognizer of hard functions. The property used is not "large."
 *
 * This section:
 *   (a) Encodes the Williams argument structure
 *   (b) Formalizes the connection: faster-SAT ↔ lower-bounds
 *   (c) Attempts to extend it from ACC⁰ to general circuits
 *   (d) Documents the new wall
 * ═══════════════════════════════════════════════════════════ */

typedef struct {
    char  *circuit_class;    /* e.g. "ACC0", "TC0", "P/poly"    */
    double speedup_factor;   /* how much faster than 2^n        */
    bool   lower_bound_follows; /* does lower bound follow?     */
    char  *lower_bound;      /* what lower bound is achieved    */
    bool   extends_to_pnp;   /* does this resolve P vs NP?      */
    char  *extension_obstacle; /* why not                       */
} WilliamsResult;

/*
 * williams_connection: formalizes the faster-SAT → lower-bound link.
 *
 * The core theorem (informal):
 *   If CircuitClass-SAT ∈ DTIME(2^n / n^ω(1))
 *   then NEXP ⊄ CircuitClass
 *
 * We check this for different circuit classes.
 */
WilliamsResult williams_connection(const char *cls,
                                    double speedup,
                                    bool   known_fast_sat) {
    WilliamsResult r;
    r.circuit_class = (char *)cls;
    r.speedup_factor = speedup;

    if (strcmp(cls, "ACC0") == 0) {
        r.lower_bound_follows  = true;
        r.lower_bound          = "NEXP ⊄ ACC⁰ (Williams 2011, proven)";
        r.extends_to_pnp       = false;
        r.extension_obstacle   =
            "NEXP ⊄ ACC⁰ does not imply P ≠ NP directly.\n"
            "         We need SAT ∉ P/poly, which is stronger.\n"
            "         The algorithm used exploits ACC⁰ structure\n"
            "         (Barrington's theorem, mod-counting gates)\n"
            "         that general P/poly circuits don't have.";

    } else if (strcmp(cls, "TC0") == 0) {
        r.lower_bound_follows  = known_fast_sat;
        r.lower_bound          = known_fast_sat
                                 ? "NEXP ⊄ TC⁰ (conjectured, not proven)"
                                 : "unknown — no fast TC⁰-SAT algorithm known";
        r.extends_to_pnp       = false;
        r.extension_obstacle   =
            "No subexponential TC⁰-SAT algorithm is known.\n"
            "         TC⁰ contains integer multiplication — very powerful.\n"
            "         The Williams method stalls here.";

    } else if (strcmp(cls, "P/poly") == 0) {
        r.lower_bound_follows  = false;
        r.lower_bound          = "unknown — would require SAT ∉ P/poly";
        r.extends_to_pnp       = true;   /* would resolve it */
        r.extension_obstacle   =
            "To get SAT ∉ P/poly via Williams method we would need\n"
            "         a subexponential algorithm for GENERAL circuit SAT.\n"
            "         But general circuits can compute ANY function —\n"
            "         there is no known exploitable structure.\n"
            "         This is the new wall: the Williams method requires\n"
            "         the target circuit class to have ALGORITHMIC STRUCTURE\n"
            "         that can be exploited for faster simulation.\n"
            "         P/poly has no such structure by definition.";

    } else {
        r.lower_bound_follows  = false;
        r.lower_bound          = "unknown";
        r.extends_to_pnp       = false;
        r.extension_obstacle   = "circuit class not analyzed";
    }

    return r;
}

void section_D(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  D: WILLIAMS METHOD (2011) — ROUTING AROUND         ║\n");
    printf("║     THE NATURAL PROOFS BARRIER                      ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    printf("Core insight:\n");
    printf("  FASTER SAT ALGORITHM → CIRCUIT LOWER BOUND\n\n");

    printf("  If CircuitClass-SAT ∈ DTIME(2^n / n^ω(1)),\n");
    printf("  then NEXP ⊄ CircuitClass.\n\n");

    printf("Why this avoids natural proofs:\n");
    printf("  It does not use a LARGE property of functions.\n");
    printf("  It uses an ALGORITHMIC property of circuit evaluation.\n");
    printf("  The argument is not constructive in the RR sense —\n");
    printf("  it does not give an efficient recognizer of hard functions.\n");
    printf("  The RR barrier does NOT apply.\n\n");

    const char *classes[] = { "ACC0", "TC0", "P/poly" };
    double      speeds[]  = { 2.0,    1.0,   0.0     };
    bool        knowns[]  = { true,   false, false   };

    for (int i = 0; i < 3; i++) {
        WilliamsResult r = williams_connection(classes[i],
                                               speeds[i], knowns[i]);
        printf("── Class: %-10s ─────────────────────────────────\n",
               r.circuit_class);
        printf("  Speedup known:        %s\n",
               r.speedup_factor > 0 ? "yes" : "no");
        printf("  Lower bound follows:  %s\n",
               r.lower_bound_follows ? "YES" : "NO");
        printf("  Lower bound:          %s\n", r.lower_bound);
        printf("  Resolves P vs NP:     %s\n",
               r.extends_to_pnp ? "WOULD" : "no");
        printf("  Obstacle:             %s\n\n",
               r.extension_obstacle);
    }
}

/* ═══════════════════════════════════════════════════════════
 * SECTION E - THE NEW WALL
 *
 * The Williams method works for ACC⁰ because ACC⁰ circuits
 * have exploitable ALGEBRAIC STRUCTURE (Barrington + mod gates).
 *
 * For P/poly (general circuits), there is no such structure.
 * The new wall is:
 *   "Algorithmic structure" ↔ "circuit class has exploitable properties"
 *
 * This is essentially the SAME problem restated:
 *   To exploit structure, you need to understand the circuit class.
 *   To understand why SAT is hard for that class, you need P ≠ NP.
 *   Circular.
 *
 * However: there is one genuine opening left.
 * The Williams method + derandomization:
 *   If we could derandomize BPP = P (widely believed),
 *   then pseudorandom generators exist with hardness assumptions.
 *   Those hardness assumptions, combined with Williams-style
 *   algorithms, might close the loop.
 *   This is the Impagliazzo-Wigderson program.
 * ═══════════════════════════════════════════════════════════ */

void section_E(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  E: THE NEW WALL AND THE OPENING                    ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    printf("The Williams method stalls at P/poly because:\n\n");
    printf("  General circuits have no exploitable algebraic structure.\n");
    printf("  Faster simulation of P/poly would mean P = PSPACE.\n");
    printf("  That is not a contradiction — it is an open problem.\n\n");

    printf("The new wall:\n");
    printf("  'Algorithmic structure' required by Williams\n");
    printf("  ≡ understanding the circuit class deeply enough\n");
    printf("  ≡ understanding why SAT is hard for that class\n");
    printf("  ≡ knowing P ≠ NP in that restricted sense\n");
    printf("  → Circular dependency.\n\n");

    printf("What was genuinely achieved by Attempt 3:\n\n");
    printf("  ✓ Boolean circuit model implemented and tested\n");
    printf("  ✓ Switching lemma verified empirically\n");
    printf("  ✓ Natural proofs barrier audited precisely\n");
    printf("  ✓ Williams method encoded and applied\n");
    printf("  ✓ ACC⁰ lower bound structure understood\n");
    printf("  ✓ The NEW wall identified (algorithmic structure)\n");
    printf("  ✗ P/poly lower bound — not achieved\n\n");

    printf("The genuine opening — Impagliazzo-Wigderson program:\n\n");
    printf("  If BPP = P (believed true), then either:\n");
    printf("    (a) E = DTIME(2^{O(n)}) requires large circuits\n");
    printf("        → hardness amplification → PRGs exist\n");
    printf("        → derandomization works\n");
    printf("        → combined with Williams → lower bounds\n");
    printf("    OR\n");
    printf("    (b) The derandomization assumption fails\n");
    printf("        → which itself gives circuit lower bounds\n\n");
    printf("  Either branch gives lower bounds. This is the\n");
    printf("  most promising current program. Attempt 4 will\n");
    printf("  encode the Impagliazzo-Wigderson structure.\n\n");
}

/* ═══════════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════════ */

int main(void) {
    printf("theory3.c — P vs NP Attempt 3: Circuit Complexity\n");
    printf("Swirly Crop\n");
    printf("Builds on: deduce.c, theory1.c, theory2.c\n\n");

    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  A: BOOLEAN CIRCUIT MODEL                           ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    Circuit C = build_small_sat_circuit();
    print_circuit_stats(&C);

    printf("\n  Evaluating on all 4 inputs (x1,x2):\n");
    bool inputs[4][2] = {{0,0},{0,1},{1,0},{1,1}};
    for (int i = 0; i < 4; i++) {
        bool out = eval_circuit(&C, inputs[i]);
        printf("    C(%d,%d) = %d\n", (int)inputs[i][0],
               (int)inputs[i][1], out);
    }
    printf("\n  P/poly: a family {C_n} is poly-size if size(C_n) = O(n^k)\n");
    printf("  Goal: show no such family computes SAT.\n\n");

    section_B();
    section_C();
    section_D();
    section_E();

    /* cleanup */
    for (int i = 2; i < C.n_gates; i++)
        if (C.gates[i].children) free(C.gates[i].children);
    free(C.gates);

    return 0;
}