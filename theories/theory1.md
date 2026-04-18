# P vs NP - Attempt 1: Diagonal Witness Argument

**Project:** Swirly Crop - P vs NP Deductive Series  
**File:** `theory1.c`  
**Built on:** `deduce.c` natural deduction infrastructure  
**Status:** `VALID STRUCTURE / UNSOUND EXECUTION`  
**Series position:** Attempt 1 of N

---

## What This Attempt Tries

The strategy is diagonalization - the same family of argument that proves the
Halting Problem undecidable and that separates deterministic time classes via
the Time Hierarchy Theorem.

The goal is to derive a contradiction from the assumption `[P = NP]¹`, then
discharge that assumption via `¬Intro` to conclude `P ≠ NP`.

The logical form is:

```
[P = NP]¹
    ...
    ⊥
──────────  ¬Intro, 1
 ¬(P = NP)
```

---

## The Argument - Step by Step

### Step 1 - Introduce `[P = NP]¹`

Assume P = NP as a dischargeable hypothesis, labelled `1`.
This is not asserted as true — it is the antecedent we will
derive a contradiction from.

### Step 2 - Enumerate poly-time machines

All polynomial-time Turing machines form a countable set.
They can be enumerated:

```
M_0, M_1, M_2, M_3, ...
```

Each `M_i` runs in time `O(n^{k_i})` for some `k_i`. This enumeration
exists as a mathematical fact — PTMs are finitely describable.

### Step 3 - Construct the diagonal language D

Define:

```
D = { x | M_x(x) rejects }
```

For each `x`, ask whether the `x`-th machine rejects its own index as input.
This is the Cantor diagonal move - `D` is designed to differ from every `M_i`
on at least one input (input `i` itself).

### Step 4 - D ∈ NP

`D` has a polynomial-time verifier:

```
V_D(x, cert):
  cert = a transcript of M_x's computation on x
  check: cert is a valid rejecting computation of M_x on x
```

Checking a computation transcript is polynomial in its length.
Therefore `D ∈ NP`.

Under `[P = NP]¹`: `D ∈ NP → D ∈ P`, so some machine `M_k` decides
`D` in polynomial time.

### Step 5 - Derive ⊥ via M_k(k)

Ask: does `M_k(k)` accept?

```
Case 1: M_k(k) accepts
  → k ∈ D  (M_k correctly decides D)
  → M_k(k) rejects  (definition of D)
  → ACCEPT ∧ REJECT = ⊥

Case 2: M_k(k) rejects
  → k ∉ D  (M_k correctly decides D)
  → M_k(k) accepts  (definition of D)
  → REJECT ∧ ACCEPT = ⊥
```

Both cases yield `⊥`. The contradiction is derived.

### Step 6 - ¬Intro: discharge `[P = NP]¹`

From `⊥` derived under `[P = NP]¹`, apply `¬Intro`:

```
⊢ ¬(P = NP)
```

i.e. `P ≠ NP`.

---

## Where It Collapses

The argument breaks at **Step 3**.

The diagonal construction requires evaluating `M_x(x)` inside `D`'s
membership check. For this to keep `D` inside a fixed complexity class,
we need a universal polynomial-time simulator `U` such that:

```
U(x, x) simulates M_x(x) within time poly(|x|)
        for ALL polynomial-time machines M_x
```

This does not exist at a fixed polynomial.

`M_x` may run in time `n^k` for arbitrarily large `k`. A universal
simulator adds overhead — even linear overhead produces:

```
T_D(n) = O(n) · O(n^k) = O(n^{k+1})
```

The diagonal language escapes the bound. `D` is not demonstrably
inside `P` or `NP` in a way that lets the diagonal argument close.

This is precisely why the **Time Hierarchy Theorem** can give:

```
DTIME(n^k) ⊊ DTIME(n^{k+1})
```

but cannot give `P ⊊ NP` - the separation between deterministic
time classes works because the diagonal stays inside the *larger*
class. Against NP, nondeterminism introduces a qualitative gap
that polynomial simulation cannot straddle.

**Baker-Gill-Solovay (1975)** formalized the deeper obstruction:
any argument of this type *relativizes* — it holds in all oracle
worlds. But P vs NP has different answers in different oracle worlds
(relative to some oracles P = NP; relative to others P ≠ NP).
Therefore no relativizing argument, including diagonalization, can
resolve it.

---

## Natural Deduction Rules Used

| Rule | Applied at |
|---|---|
| `→Intro` | Discharging assumptions in subderivations |
| `∧Intro` | Joining correctness and runtime conditions |
| `∀Intro` | Generalizing over arbitrary L ∈ NP |
| `¬Intro` | Discharging `[P=NP]¹` from ⊥ — Step 6 |

The new rule introduced in this attempt versus `deduce.c`:

```
  [φ]^n
   ...
   ⊥
──────────  ¬Intro, n
  ¬φ
```

Discharging of `φ` is a permission not a requirement — consistent
with the textbook formulation (an assumption need not be present to
apply a rule).

---

## What the Code Does

`theory1.c` encodes the full 6-step derivation as executable C:

- `step1_assume()` - introduces the hypothesis
- `step2_enumerate()` - builds the finite machine table
- `step3_diagonal()` - evaluates D membership for x = 0..3
- `step4_D_in_NP()` - argues D ∈ NP and D ∈ P under assumption
- `step5_contradiction()` - runs the diagonal query, derives ⊥
- `step6_neg_intro()` - applies ¬Intro, concludes ¬(P=NP)
- `report_wall()` - documents precisely where soundness fails

The `Judgement.valid` field tracks whether each step is sound.
The structural derivation completes - the code prints `(¬P=NP)` -
but `report_wall()` immediately documents that the diagonal
construction is unsound, keeping the attempt honest.

---

## Verdict

| Property | Result |
|---|---|
| Logical structure | Valid |
| ND rules applied correctly | Yes |
| Mathematical content sound | No |
| Barrier hit | Relativization (Baker-Gill-Solovay) |
| Collapses at | Step 3 - diagonal simulation overhead |

The skeleton is right. The content fails at the only place that matters.

---

## What Attempt 2 Will Try

Diagonalization is a *relativizing* technique. Attempt 2 will move to
a **non-relativizing** approach: circuit complexity lower bounds.

The strategy will be to show that no polynomial-size Boolean circuit
family can compute SAT - which would imply SAT ∉ P/poly, a stronger
statement than P ≠ NP but achievable (in principle) without
diagonalization. The key tool will be the **switching lemma** and
attempts at superpolynomial lower bounds on circuit depth/size.

This is the approach Razborov pursued in the 1980s for monotone circuits —
it succeeded for specific problems (clique) but hit a new wall
(the natural proofs barrier) for general circuits.

Attempt 2 will encode that structure and document where it collapses.

---

*Swirly Crop(chant[ψαλμός]) - P vs NP Deductive Series*