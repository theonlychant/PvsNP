/*
 * deduce.c
 *
 * A formal encoding of the Cook-Levin natural deduction derivation.
 * Proves: (SAT ∈ P) → (∀L ∈ NP. L ∈ P)
 *
 * Each function corresponds to a rule or premise in the derivation.
 * This is not a general SAT solver — it encodes the logical skeleton
 * of the proof, making each inference step explicit and checkable.
 *
 * Premises:
 *   P1 — NP Reducibility      : ∀L ∈ NP. ∃f_L poly-time. x∈L ↔ f_L(x)∈SAT
 *   P2 — Composition Closure  : poly-time ∘ poly-time = poly-time
 *   P3 — SAT Hypothesis       : [SAT ∈ P]¹  (discharged assumption)
 *   P4 — Definition of P      : ∃M poly-time Turing machine deciding L
 *   P5 — Correctness of chain : A_SAT(f_L(x)) accepts ↔ x ∈ L
 *
 * Author: Swirly Crop(chant[ψαλμός])
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ─────────────────────────────────────────────
 * TYPE DEFINITIONS
 * ───────────────────────────────────────────── */

/*
 * A Formula represents a propositional/predicate logic sentence.
 * In the derivation, formulas are the nodes of the proof tree.
 */
typedef struct Formula {
    char *name;         /* human-readable label */
    bool  truth;        /* current truth value under assignment */
    int   label;        /* discharge label (0 = not discharged) */
} Formula;

/*
 * A Language encodes membership: a function from input to bool.
 * In the derivation, L ∈ NP means L has a poly-time verifier.
 */
typedef struct Language {
    char *name;
    bool (*decide)(const char *input);     /* decider (if in P) */
    char *(*reduce)(const char *input);    /* reduction f_L to SAT */
} Language;

/*
 * A Judgement is a sequent: a set of assumptions proving a conclusion.
 * Corresponds to a line in the natural deduction tree.
 */
typedef struct Judgement {
    char *assumption;
    char *conclusion;
    char *rule;          /* which ND rule was applied */
    int   discharge;     /* label discharged here, 0 if none */
} Judgement;


/* ─────────────────────────────────────────────
 * PREMISE 1 — NP REDUCIBILITY
 * ∀L ∈ NP. ∃f_L poly-time. x ∈ L ↔ f_L(x) ∈ SAT
 * ───────────────────────────────────────────── */

/*
 * apply_reduction: applies f_L to input x, producing a SAT instance.
 * In a real implementation this would be the Cook-Levin tableau
 * construction encoding the NP verifier as a Boolean circuit.
 *
 * Here we encode the *structure* of the reduction: the output is
 * a SAT formula string that is satisfiable iff x ∈ L.
 */
char *apply_reduction(Language *L, const char *x) {
    if (!L || !L->reduce) {
        fprintf(stderr, "[P1] No reduction defined for language %s\n",
                L ? L->name : "NULL");
        return NULL;
    }
    char *sat_instance = L->reduce(x);
    printf("[P1] Reduction f_%s(%s) → SAT instance: %s\n",
           L->name, x, sat_instance);
    return sat_instance;
}


/* ─────────────────────────────────────────────
 * PREMISE 2 — POLYNOMIAL COMPOSITION CLOSURE
 * O(n^j) ∘ O(n^k) = O(n^(j*k))
 * ───────────────────────────────────────────── */

typedef struct PolyTime {
    int  exponent;       /* p(n) = n^exponent */
    char *label;         /* human label */
} PolyTime;

/*
 * compose_poly: computes the composed runtime of g∘f.
 * If f: O(n^j) and g: O(n^k), then g(f(x)): O(n^(j*k)).
 * This is P2 — closure of polynomial time under composition.
 */
PolyTime compose_poly(PolyTime f, PolyTime g) {
    PolyTime composed;
    composed.exponent = f.exponent * g.exponent;
    composed.label    = "composed_pipeline";
    printf("[P2] Composition: O(n^%d) ∘ O(n^%d) = O(n^%d)\n",
           g.exponent, f.exponent, composed.exponent);
    return composed;
}


/* ─────────────────────────────────────────────
 * PREMISE 3 — SAT HYPOTHESIS  [SAT ∈ P]¹
 * Discharged assumption — introduced here, discharged in →Intro
 * ───────────────────────────────────────────── */

/*
 * A mock polynomial-time SAT solver.
 * In reality no such solver is known — this represents the
 * *hypothetical* A_SAT that exists if SAT ∈ P.
 * The discharge label ¹ tracks that this is an assumption.
 */
typedef struct SATSolver {
    char     *name;
    PolyTime  runtime;
    bool    (*solve)(const char *formula);   /* the hypothetical solver */
} SATSolver;

bool mock_sat_solve(const char *formula) {
    /*
     * Placeholder: in a real proof assistant this would be
     * the assumed poly-time oracle. The reduction f_L guarantees
     * f_L(x) ∈ SAT ↔ x ∈ L, so A_SAT must accept iff the
     * formula encodes a satisfiable instance. Here we simulate
     * a correct oracle: non-empty formula = satisfiable (mock).
     * The correctness of the biconditional chain is what P5 asserts.
     */
    printf("[P3] A_SAT evaluating formula: %s\n", formula);
    return (strlen(formula) > 0);   /* mock: non-empty → satisfiable */
}

SATSolver make_sat_hypothesis(void) {
    SATSolver A_SAT;
    A_SAT.name         = "A_SAT [assumed, label=1]";
    A_SAT.runtime      = (PolyTime){ .exponent = 3, .label = "poly" };
    A_SAT.solve        = mock_sat_solve;
    printf("[P3] Assumption introduced: [SAT ∈ P]¹\n");
    printf("     A_SAT runtime: O(n^%d) — hypothetical\n",
           A_SAT.runtime.exponent);
    return A_SAT;
}


/* ─────────────────────────────────────────────
 * PREMISE 4 — DEFINITION OF P
 * L ∈ P iff ∃ poly-time TM deciding L
 * ───────────────────────────────────────────── */

typedef struct PMembership {
    char     *language;
    PolyTime  runtime;
    bool      certified;    /* both conditions met: correct + poly-time */
} PMembership;

/*
 * certify_P: verifies both conditions for L ∈ P.
 *   1. The machine decides L correctly (Premise 5 establishes this)
 *   2. The machine runs in polynomial time (Premise 2 establishes this)
 */
PMembership certify_P(const char *lang_name, PolyTime runtime, bool correct) {
    PMembership cert;
    cert.language  = (char *)lang_name;
    cert.runtime   = runtime;
    cert.certified = correct && (runtime.exponent > 0);
    printf("[P4] Certifying %s ∈ P: correct=%s, runtime=O(n^%d) → %s\n",
           lang_name,
           correct ? "true" : "false",
           runtime.exponent,
           cert.certified ? "CERTIFIED" : "FAILED");
    return cert;
}


/* ─────────────────────────────────────────────
 * PREMISE 5 — CORRECTNESS OF COMPOSED DECIDER
 * A_SAT(f_L(x)) accepts ↔ x ∈ L
 * ───────────────────────────────────────────── */

/*
 * check_biconditional: verifies P5 for a given input x.
 * Chains: x ∈ L ↔ f_L(x) ∈ SAT ↔ A_SAT(f_L(x)) accepts.
 * Returns true if the composed machine correctly decides x.
 */
bool check_biconditional(Language    *L,
                          SATSolver   *A_SAT,
                          const char  *x,
                          bool         x_in_L) {
    char *sat_instance  = apply_reduction(L, x);
    bool  sat_accepts   = A_SAT->solve(sat_instance);

    printf("[P5] x∈L=%s, A_SAT(f_L(x))=%s → biconditional %s\n",
           x_in_L     ? "true" : "false",
           sat_accepts ? "true" : "false",
           (x_in_L == sat_accepts) ? "HOLDS" : "VIOLATED");

    free(sat_instance);
    return (x_in_L == sat_accepts);
}


/* ─────────────────────────────────────────────
 * NATURAL DEDUCTION RULES
 * ───────────────────────────────────────────── */

/*
 * nd_and_intro: ∧Intro
 * From proof of φ and proof of ψ, derive φ ∧ ψ.
 *
 *   φ    ψ
 *  ───────── ∧Intro
 *   φ ∧ ψ
 */
Judgement nd_and_intro(Judgement phi, Judgement psi) {
    Judgement conj;
    size_t len = strlen(phi.conclusion) + strlen(psi.conclusion) + 4;
    conj.conclusion = malloc(len);
    snprintf(conj.conclusion, len, "(%s ∧ %s)",
             phi.conclusion, psi.conclusion);
    conj.assumption = phi.assumption;
    conj.rule       = "∧Intro";
    conj.discharge  = 0;
    printf("[∧Intro] %s , %s ⊢ %s\n",
           phi.conclusion, psi.conclusion, conj.conclusion);
    return conj;
}

/*
 * nd_implies_intro: →Intro
 * From derivation of ψ under assumption φ, derive φ → ψ.
 * Discharges the assumption marked with the given label.
 *
 *   [φ]^n
 *    ...
 *     ψ
 *  ───────── →Intro, n
 *   φ → ψ
 */
Judgement nd_implies_intro(const char *phi,
                            Judgement   psi,
                            int         label) {
    Judgement impl;
    size_t len = strlen(phi) + strlen(psi.conclusion) + 6;
    impl.conclusion = malloc(len);
    snprintf(impl.conclusion, len, "(%s → %s)", phi, psi.conclusion);
    impl.assumption = "∅";
    impl.rule       = "→Intro";
    impl.discharge  = label;
    printf("[→Intro] Discharging assumption [%s]^%d\n", phi, label);
    printf("         ⊢ %s\n", impl.conclusion);
    return impl;
}

/*
 * nd_forall_intro: ∀Intro
 * From derivation of P(x) for arbitrary x, derive ∀x.P(x).
 * x must not appear free in any undischarged assumption.
 */
Judgement nd_forall_intro(const char *var, Judgement body) {
    Judgement univ;
    size_t len = strlen(var) + strlen(body.conclusion) + 8;
    univ.conclusion = malloc(len);
    snprintf(univ.conclusion, len, "(∀%s. %s)", var, body.conclusion);
    univ.assumption = body.assumption;
    univ.rule       = "∀Intro";
    univ.discharge  = 0;
    printf("[∀Intro] Arbitrary %s: ⊢ %s\n", var, univ.conclusion);
    return univ;
}


/* ─────────────────────────────────────────────
 * THE DERIVATION
 * Proves: ⊢ (SAT ∈ P) → (∀L ∈ NP. L ∈ P)
 * ───────────────────────────────────────────── */

void run_derivation(void) {
    printf("\n══════════════════════════════════════════\n");
    printf("  Cook-Levin Natural Deduction Derivation\n");
    printf("  ⊢ (SAT ∈ P) → (∀L ∈ NP. L ∈ P)\n");
    printf("══════════════════════════════════════════\n\n");

    /* ── Step 1: Introduce [SAT ∈ P]¹  (P3) ── */
    SATSolver A_SAT = make_sat_hypothesis();
    printf("\n");

    /* ── Step 2: Pick arbitrary L ∈ NP ── */
    printf("--- Picking arbitrary L ∈ NP ---\n");

    /* Mock reduction for a language "3COLOR" (graph 3-colorability) */
    Language L_example;
    L_example.name   = "3COLOR";
    L_example.decide = NULL;    /* unknown — that's what we're proving */
    L_example.reduce = (char *(*)(const char *))strdup; /* mock: identity */
    printf("[∀Elim] Let L = %s, L ∈ NP\n\n", L_example.name);

    /* ── Step 3: Apply P1 — get reduction f_L ── */
    printf("--- Applying P1: NP Reducibility ---\n");
    PolyTime f_L_runtime = { .exponent = 2, .label = "f_3COLOR" };
    printf("[P1] f_%s exists, runtime O(n^%d)\n\n",
           L_example.name, f_L_runtime.exponent);

    /* ── Step 4: Apply P2 — compose runtimes ── */
    printf("--- Applying P2: Composition Closure ---\n");
    PolyTime composed = compose_poly(f_L_runtime, A_SAT.runtime);
    printf("\n");

    /* ── Step 5: Apply P5 — check biconditional ── */
    printf("--- Applying P5: Biconditional Correctness ---\n");
    const char *test_input = "example_graph";
    bool x_in_L = true;   /* stipulated for this instance */
    bool correct = check_biconditional(&L_example, &A_SAT,
                                        test_input, x_in_L);
    printf("\n");

    /* ── Step 6: Apply P4 — certify L ∈ P ── */
    printf("--- Applying P4: Certify L ∈ P ---\n");
    PMembership cert = certify_P(L_example.name, composed, correct);
    printf("\n");

    /* ── Step 7: ∧Intro — both conditions met ── */
    printf("--- ∧Intro: joining correctness and runtime ---\n");
    Judgement j_correct  = { .conclusion = "A_SAT∘f_L decides L",
                              .assumption = "[SAT∈P]¹",
                              .rule = "P5", .discharge = 0 };
    Judgement j_runtime  = { .conclusion = "A_SAT∘f_L ∈ poly-time",
                              .assumption = "[SAT∈P]¹",
                              .rule = "P2", .discharge = 0 };
    Judgement j_and      = nd_and_intro(j_correct, j_runtime);
    printf("\n");

    /* ── Step 8: →Intro — L ∈ P from conjunction ── */
    printf("--- →Intro from conjunction to L ∈ P ---\n");
    Judgement j_L_in_P   = { .conclusion = cert.certified
                                           ? "L ∈ P" : "L ∉ P (cert failed)",
                              .assumption = "[SAT∈P]¹",
                              .rule = "P4", .discharge = 0 };
    printf("[def P] %s → %s\n", j_and.conclusion, j_L_in_P.conclusion);
    printf("\n");

    /* ── Step 9: ∀Intro — generalize over L ── */
    printf("--- ∀Intro: L was arbitrary ---\n");
    Judgement j_forall   = nd_forall_intro("L∈NP", j_L_in_P);
    printf("\n");

    /* ── Step 10: →Intro — discharge [SAT ∈ P]¹ ── */
    printf("--- →Intro: discharge assumption [SAT∈P]¹ ---\n");
    Judgement j_final    = nd_implies_intro("SAT∈P", j_forall, 1);

    /* ── Result ── */
    printf("\n══════════════════════════════════════════\n");
    printf("  DERIVED: %s\n", j_final.conclusion);
    printf("  Rule: %s, discharged label %d\n",
           j_final.rule, j_final.discharge);
    printf("══════════════════════════════════════════\n\n");

    printf("NOTE: The assumption [SAT∈P]¹ has been discharged.\n");
    printf("      This proof does not assert SAT ∈ P.\n");
    printf("      It proves the implication — the logical skeleton\n");
    printf("      of why SAT is the right problem to focus on.\n\n");

    /* cleanup */
    free(j_and.conclusion);
    free(j_forall.conclusion);
    free(j_final.conclusion);
}


/* ─────────────────────────────────────────────
 * MAIN
 * ───────────────────────────────────────────── */

int main(void) {
    printf("deduce.c — Cook-Levin Implication in Natural Deduction\n");
    printf("Swirly Crop\n\n");

    run_derivation();

    return 0;
}