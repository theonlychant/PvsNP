/*
 * theory1.c
 * P vs NP - Attempt 1: Deductive Separation via Diagonal Witness
 *
 * Strategy:
 *   Attempt to prove P ≠ NP by constructing a diagonal argument:
 *   assume P = NP, then derive a contradiction via a language
 *   that cannot be decided by any polynomial-time machine.
 *
 *   This mirrors the structure of Cantor diagonalization and
 *   the proof that the Halting Problem is undecidable — adapted
 *   to the polynomial-time setting.
 *
 * The Argument:
 *   (1) Assume [P = NP]¹
 *   (2) Enumerate all poly-time machines M_1, M_2, M_3, ...
 *   (3) Construct diagonal language D:
 *         x ∈ D ↔ M_x(x) rejects  (within poly-time bound)
 *   (4) D ∈ NP  (it has a verifier — trivially, its own decider)
 *   (5) If P = NP, then D ∈ P, so some M_k decides D in poly-time
 *   (6) Ask: does M_k(k) accept?
 *         If yes → k ∈ D → M_k(k) rejects     CONTRADICTION
 *         If no  → k ∉ D → M_k(k) accepts      CONTRADICTION
 *   (7) Contradiction discharges [P = NP]¹ → P ≠ NP
 *
 * Where This Attempt Fails:
 *   Step (3) is the wall. Unlike Turing machines, poly-time
 *   machines cannot simulate ALL other poly-time machines within
 *   a fixed polynomial bound. The diagonal language D as constructed
 *   may itself require superpolynomial time to evaluate, breaking
 *   the diagonalization. This is exactly the "time hierarchy"
 *   problem — and why the Time Hierarchy Theorem (which does work
 *   for separating P from EXP) cannot be tightened to separate P
 *   from NP. The simulation overhead collapses the argument.
 *
 *   This is documented formally in the WALL section below.
 *   The code runs the argument as far as it holds, then halts
 *   at the contradiction with an explicit failure report.
 *
 * Relationship to deduce.c:
 *   Reuses Judgement, PolyTime, nd_and_intro, nd_implies_intro,
 *   nd_forall_intro, and the ND rule infrastructure.
 *   Adds: negation (¬Intro via contradiction), enumeration,
 *   and the diagonal construction.
 *
 * Author: Swirly Crop
 * Series: P vs NP Deductive Attempts - #1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════
 * REUSED INFRASTRUCTURE FROM deduce.c
 * ═══════════════════════════════════════════════ */

typedef struct PolyTime {
    int   exponent;
    char *label;
} PolyTime;

typedef struct Judgement {
    char *assumption;
    char *conclusion;
    char *rule;
    int   discharge;
    bool  valid;       /* NEW: tracks whether this step is sound */
} Judgement;

Judgement nd_and_intro(Judgement phi, Judgement psi) {
    Judgement conj;
    size_t len       = strlen(phi.conclusion) + strlen(psi.conclusion) + 12;
    conj.conclusion  = malloc(len);
    snprintf(conj.conclusion, len, "(%s ∧ %s)",
             phi.conclusion, psi.conclusion);
    conj.assumption  = phi.assumption;
    conj.rule        = "∧Intro";
    conj.discharge   = 0;
    conj.valid       = phi.valid && psi.valid;
    printf("  [∧Intro] %s\n", conj.conclusion);
    return conj;
}

Judgement nd_implies_intro(const char *phi, Judgement psi, int label) {
    Judgement impl;
    size_t len      = strlen(phi) + strlen(psi.conclusion) + 10;
    impl.conclusion = malloc(len);
    snprintf(impl.conclusion, len, "(%s → %s)", phi, psi.conclusion);
    impl.assumption = "∅";
    impl.rule       = "→Intro";
    impl.discharge  = label;
    impl.valid      = psi.valid;
    printf("  [→Intro] Discharge [%s]^%d\n", phi, label);
    printf("           ⊢ %s\n", impl.conclusion);
    return impl;
}

Judgement nd_forall_intro(const char *var, Judgement body) {
    Judgement univ;
    size_t len      = strlen(var) + strlen(body.conclusion) + 12;
    univ.conclusion = malloc(len);
    snprintf(univ.conclusion, len, "(∀%s. %s)", var, body.conclusion);
    univ.assumption = body.assumption;
    univ.rule       = "∀Intro";
    univ.discharge  = 0;
    univ.valid      = body.valid;
    printf("  [∀Intro] ⊢ %s\n", univ.conclusion);
    return univ;
}

/* NEW: ¬Intro - derive ¬φ from a derivation of ⊥ under [φ]^n */
Judgement nd_neg_intro(const char *phi, int label) {
    Judgement neg;
    size_t len      = strlen(phi) + 6;
    neg.conclusion  = malloc(len);
    snprintf(neg.conclusion, len, "(¬%s)", phi);
    neg.assumption  = "∅";
    neg.rule        = "¬Intro";
    neg.discharge   = label;
    neg.valid       = true;   /* validity depends on the contradiction below */
    printf("  [¬Intro] Discharge [%s]^%d → ⊢ %s\n",
           phi, label, neg.conclusion);
    return neg;
}

/* ═══════════════════════════════════════════════
 * NEW TYPES FOR ATTEMPT 1
 * ═══════════════════════════════════════════════ */

/*
 * A poly-time machine in the enumeration.
 * M_i is identified by index i, has a runtime bound, and a decider.
 * The decider returns: 1 = accept, 0 = reject, -1 = exceeds time bound.
 */
typedef struct PolyMachine {
    int   index;
    PolyTime runtime;
    int (*run)(int input_index);    /* run machine on input encoded as int */
} PolyMachine;

/*
 * Result of the diagonal query.
 * Records the contradiction explicitly.
 */
typedef struct DiagResult {
    int   machine_index;    /* k: the index of the machine claiming to decide D */
    int   accepts;          /* what M_k(k) returns */
    bool  contradiction;    /* was a contradiction derived? */
    char *reason;           /* explanation */
} DiagResult;

/* ═══════════════════════════════════════════════
 * STEP 1 - ASSUMPTION [P = NP]¹
 * ═══════════════════════════════════════════════ */

void step1_assume(void) {
    printf("┌─ Step 1: Introduce assumption [P = NP]¹\n");
    printf("│  This is the antecedent we will derive ⊥ from.\n");
    printf("│  If we succeed, ¬Intro discharges it → P ≠ NP.\n");
    printf("└─ [P = NP]¹ - introduced, label=1\n\n");
}

/* ═══════════════════════════════════════════════
 * STEP 2 - ENUMERATE POLY-TIME MACHINES
 * ═══════════════════════════════════════════════ */

/*
 * Mock machines M_0, M_1, M_2, ... representing the enumeration
 * of all polynomial-time Turing machines.
 * In reality this enumeration exists - PTMs are a countable set.
 * We encode three concrete ones for the diagonal construction.
 */

int machine_0(int x) { return (x % 2 == 0) ? 1 : 0; }   /* accepts evens */
int machine_1(int x) { return (x % 3 == 0) ? 1 : 0; }   /* accepts multiples of 3 */
int machine_2(int x) { return 1; }                        /* accepts everything */
int machine_3(int x) { return 0; }                        /* rejects everything */

#define NUM_MACHINES 4

PolyMachine machines[NUM_MACHINES];

void step2_enumerate(void) {
    printf("┌─ Step 2: Enumerate poly-time machines M_0 ... M_%d\n",
           NUM_MACHINES - 1);

    machines[0] = (PolyMachine){ 0, { 1, "O(n)"   }, machine_0 };
    machines[1] = (PolyMachine){ 1, { 2, "O(n^2)" }, machine_1 };
    machines[2] = (PolyMachine){ 2, { 1, "O(n)"   }, machine_2 };
    machines[3] = (PolyMachine){ 3, { 1, "O(n)"   }, machine_3 };

    for (int i = 0; i < NUM_MACHINES; i++) {
        printf("│  M_%d: runtime %s, M_%d(%d) = %s\n",
               i, machines[i].runtime.label,
               i, i,
               machines[i].run(i) ? "ACCEPT" : "REJECT");
    }
    printf("└─ Enumeration complete (finite prefix shown)\n\n");
}

/* ═══════════════════════════════════════════════
 * STEP 3 - DIAGONAL LANGUAGE D
 * x ∈ D ↔ M_x(x) rejects
 * ═══════════════════════════════════════════════ */

/*
 * diag_member: decides membership in D for input x.
 * D is defined by: x ∈ D ↔ M_x(x) rejects (= returns 0).
 *
 * CRITICAL NOTE: This function assumes we can evaluate M_x(x)
 * within a fixed polynomial bound. This is the step that
 * breaks down - see WALL below.
 */
bool diag_member(int x) {
    if (x >= NUM_MACHINES) {
        /* Beyond our finite enumeration — undefined in this simulation */
        printf("  [D] x=%d beyond enumeration — undefined\n", x);
        return false;
    }
    int result = machines[x].run(x);
    printf("  [D] M_%d(%d) = %s → x=%d %s D\n",
           x, x,
           result ? "ACCEPT" : "REJECT",
           x,
           result ? "∉" : "∈");
    return (result == 0);   /* x ∈ D iff M_x rejects x */
}

void step3_diagonal(void) {
    printf("┌─ Step 3: Construct diagonal language D\n");
    printf("│  D = { x | M_x(x) rejects }\n");
    printf("│  Membership check for x = 0..%d:\n", NUM_MACHINES - 1);
    for (int i = 0; i < NUM_MACHINES; i++) {
        bool in_D = diag_member(i);
        printf("│    %d %s D\n", i, in_D ? "∈" : "∉");
    }
    printf("└─ D constructed\n\n");
}

/* ═══════════════════════════════════════════════
 * STEP 4 - D ∈ NP
 * D has a trivial poly-time verifier under [P=NP]¹
 * ═══════════════════════════════════════════════ */

void step4_D_in_NP(void) {
    printf("┌─ Step 4: D ∈ NP\n");
    printf("│  Verifier V_D(x, cert):\n");
    printf("│    cert = the accepting computation of M_x on x (if any)\n");
    printf("│    V_D checks cert is valid and M_x(x) rejects\n");
    printf("│  This is poly-time checkable → D ∈ NP\n");
    printf("│\n");
    printf("│  Under [P = NP]¹: D ∈ NP → D ∈ P\n");
    printf("│  So ∃ some M_k in our enumeration that decides D in poly-time\n");
    printf("└─ D ∈ P assumed, M_k exists\n\n");
}

/* ═══════════════════════════════════════════════
 * STEP 5 - DERIVE CONTRADICTION
 * Ask: does M_k(k) accept?
 * ═══════════════════════════════════════════════ */

DiagResult step5_contradiction(void) {
    printf("┌─ Step 5: Derive contradiction via M_k(k)\n");
    printf("│  M_k decides D. Ask: does M_k(k) accept?\n│\n");

    /*
     * In the actual argument k is the index of the machine deciding D.
     * We use machine index 0 as our concrete M_k for illustration.
     * The contradiction is structural - it holds for any k.
     */
    int k       = 0;
    int accepts = machines[k].run(k);

    DiagResult result;
    result.machine_index = k;
    result.accepts       = accepts;
    result.contradiction = true;

    if (accepts) {
        printf("│  M_%d(%d) = ACCEPT\n", k, k);
        printf("│  → k ∈ D (by M_k deciding D correctly)\n");
        printf("│  → M_%d(%d) should REJECT (by definition of D)\n", k, k);
        printf("│  → ACCEPT ∧ REJECT = ⊥\n");
        result.reason = "M_k accepts → k∈D → M_k should reject: contradiction";
    } else {
        printf("│  M_%d(%d) = REJECT\n", k, k);
        printf("│  → k ∉ D (by M_k deciding D correctly)\n");
        printf("│  → M_%d(%d) should ACCEPT (by definition of D)\n", k, k);
        printf("│  → REJECT ∧ ACCEPT = ⊥\n");
        result.reason = "M_k rejects → k∉D → M_k should accept: contradiction";
    }

    printf("│\n│  ⊥ derived under [P = NP]¹\n");
    printf("└─ Contradiction: %s\n\n", result.reason);
    return result;
}

/* ═══════════════════════════════════════════════
 * STEP 6 — ¬Intro: DISCHARGE [P=NP]¹ → P≠NP
 * ═══════════════════════════════════════════════ */

Judgement step6_neg_intro(DiagResult diag) {
    printf("┌─ Step 6: ¬Intro — discharge [P=NP]¹\n");
    if (!diag.contradiction) {
        printf("│  ERROR: No contradiction was derived. ¬Intro fails.\n");
        printf("└─ ATTEMPT FAILED at ¬Intro\n\n");
        Judgement fail = { "FAILED", "FAILED", "FAILED", 0, false };
        return fail;
    }
    Judgement pnp = nd_neg_intro("P=NP", 1);
    printf("└─ ⊢ (¬(P=NP)) i.e. P ≠ NP\n\n");
    return pnp;
}

/* ═══════════════════════════════════════════════
 * THE WALL — WHERE THIS ATTEMPT COLLAPSES
 * ═══════════════════════════════════════════════ */

void report_wall(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║              WHERE ATTEMPT 1 COLLAPSES              ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    printf("The argument breaks at Step 3: the diagonal construction.\n\n");

    printf("The diagonalization works cleanly for Turing machines\n");
    printf("(undecidability of Halting) and for time classes separated\n");
    printf("by more than a polynomial factor (Time Hierarchy Theorem).\n\n");

    printf("But for P vs NP the simulation overhead is fatal:\n\n");

    printf("  To evaluate M_x(x) inside D's membership check,\n");
    printf("  we need a universal poly-time simulator U such that\n");
    printf("  U(x, x) runs in time poly(|x|) for ALL poly-time M_x.\n\n");

    printf("  The problem: M_x may run in time n^k for arbitrarily\n");
    printf("  large k. A universal simulator that handles ALL of them\n");
    printf("  within a single fixed polynomial p(n) does not exist\n");
    printf("  without a polynomial slowdown that blows the bound.\n\n");

    printf("  Concretely: if M_x runs in n^k steps and we simulate\n");
    printf("  it with overhead O(n), the diagonal language D ends up\n");
    printf("  requiring O(n^(k+1)) time — outside the original bound.\n");
    printf("  Diagonalization escapes the class it is trying to separate.\n\n");

    printf("  This is why the Time Hierarchy Theorem gives:\n");
    printf("    DTIME(n^k) ⊊ DTIME(n^(k+1))\n");
    printf("  but cannot give:\n");
    printf("    P ⊊ NP\n");
    printf("  — nondeterminism introduces a qualitative gap that\n");
    printf("  polynomial simulation cannot capture.\n\n");

    printf("  Baker-Gill-Solovay (1975) formalized this: any proof\n");
    printf("  technique based purely on diagonalization relativizes,\n");
    printf("  meaning it holds in all oracle worlds — but P vs NP\n");
    printf("  has different answers in different oracle worlds, so\n");
    printf("  no relativizing argument can resolve it.\n\n");

    printf("STATUS: Attempt 1 — VALID STRUCTURE, UNSOUND EXECUTION\n");
    printf("        The ND skeleton is correct. The diagonal construction\n");
    printf("        fails to stay within polynomial time. The wall is\n");
    printf("        the simulation overhead and relativization barrier.\n\n");

    printf("NEXT:   Attempt 2 will try a different strategy — circuit\n");
    printf("        complexity lower bounds (non-relativizing approach).\n\n");
}

/* ═══════════════════════════════════════════════
 * MAIN DERIVATION
 * ═══════════════════════════════════════════════ */

void run_theory1(void) {
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║   P vs NP — Attempt 1: Diagonal Witness Argument    ║\n");
    printf("║   Goal: derive ⊥ under [P=NP]¹, conclude P ≠ NP    ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    step1_assume();
    step2_enumerate();
    step3_diagonal();
    step4_D_in_NP();

    DiagResult diag = step5_contradiction();
    Judgement  neg  = step6_neg_intro(diag);

    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║                  DERIVATION RESULT                  ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    if (neg.valid) {
        printf("  Structural conclusion: %s\n", neg.conclusion);
        printf("  Rule: %s, discharged label: %d\n\n",
               neg.rule, neg.discharge);
        printf("  *** The structural derivation succeeds ***\n");
        printf("  *** The diagonal construction does NOT ***\n\n");
    }

    report_wall();

    /* cleanup */
    if (neg.conclusion && strcmp(neg.conclusion, "FAILED") != 0)
        free(neg.conclusion);
}

/* ═══════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════ */

int main(void) {
    printf("theory1.c — P vs NP Deductive Attempt 1\n");
    printf("Swirly Crop\n");
    printf("Built on deduce.c natural deduction infrastructure\n");

    run_theory1();

    return 0;
}