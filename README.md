# P ≠ NP - A Deductive Series

**Swirly Crop** · Systems Engineering · Computational Logic Research

---

> *"The question is not whether the problem is hard.*
> *The question is which kind of hard it is, and whether*
> *that kind has a handle."*

---

## What This Is

This repository is a structured, incremental attempt to approach
the P vs NP problem through formal deductive methods — natural
deduction, first-order logic, second-order logic, circuit complexity,
and descriptive complexity. Each attempt is implemented in C and
documented in a corresponding markdown file.

This is not a claimed proof. It is a proof *search* - built in public,
with every failure documented as precisely as every partial success.
The series tracks which barriers have been hit, which have been routed
around, and where the remaining obstacle is now located.

After five attempts, the remaining obstacle has a name, a structure,
and a concrete game whose winning strategy is the proof.

---

## Repository Structure

```
.
├── deduce.c          ← Natural deduction infrastructure
├── theory1.c         ← Attempt 1: Diagonalization
├── theory1.md        ← Attempt 1: Documentation
├── theory2.c         ← Attempt 2: FOL expression
├── theory2.md        ← Attempt 2: Documentation
├── theory3.c         ← Attempt 3: Circuit complexity + Williams method
├── theory3.md        ← Attempt 3: Documentation
├── theory4.c         ← Attempt 4: Second-order logic + categoricity
├── theory4.md        ← Attempt 4: Documentation
├── proof4.md         ← Forward path: from determination to descriptive complexity
├── theory5.c         ← Attempt 5: Descriptive complexity, FO(LFP) vs ∃SO
├── theory5.md        ← Attempt 5: Documentation
└── README.md         ← This file
```

---

## The Series

### Foundation - [`deduce.c`](./tests/deduce.c)

Natural deduction infrastructure used throughout the series.
Implements the Cook-Levin implication as a formal natural deduction
derivation:

```
⊢ (SAT ∈ P) → (∀L ∈ NP. L ∈ P)
```

Rules implemented: `∧Intro`, `→Intro`, `∀Intro`, `¬Intro`.
Each rule is a C function. The proof tree is executable.

---

### Attempt 1 - [`theory1.c`](./theories/theory1.c) · [`theory1.md`](./theories/theory1.md)

**Strategy:** Diagonalization — assume `[P = NP]¹`, construct a
diagonal language D, derive `⊥`, discharge via `¬Intro`.

**Barrier hit:** Relativization (Baker-Gill-Solovay 1975).

The diagonal language D requires a universal polynomial-time
simulator for all poly-time machines within a fixed polynomial
bound. This does not exist. The simulation overhead causes D to
escape the class it was supposed to stay in.

**Status:** `VALID STRUCTURE / UNSOUND EXECUTION`

The ND skeleton compiles and runs. The C code prints the derived
judgement `⊢ (¬P=NP)` and immediately documents why it is unsound.

---

### Attempt 2 - [`theory2.c`](./theories/theory2.c) · [`theory2.md`](./theories/theory2.md)

**Strategy:** Express P ≠ NP precisely in first-order logic.
Build a full FOL engine: typed terms, formula trees, a multi-sort
domain structure, a recursive satisfaction relation `M ⊨ φ[v]`.

**The sentence:**
```
∃L [ InNP(L) ∧ ¬InP(L) ]
```

**Barrier hit:** Arithmetic ceiling + model-theoretic obstacle.

`InP(L)` and `InNP(L)` require `|x|^k` time bounds — pulling in
Peano Arithmetic. The sentence is FOL+PA, not pure FOL. Gödel
completeness then blocks a proof: the sentence is not valid in
all models (oracle models where P = NP exist), so FOL cannot
derive it from standard complexity axioms.

**Status:** `EXPRESSION ACHIEVED / PROOF IMPOSSIBLE IN PURE FOL`

The `arithmetic_required` flag propagates honestly through every
subformula of the satisfaction tree. The sentence evaluates to
`TRUE` in the mock structure M with witness `3COLOR`.

---

### Attempt 3 - [`theory3.c`](./theories/theory3.c) · [`theory3.md`](./theories/theory3.md)

**Strategy:** Circuit complexity lower bounds. Show no polynomial-size
Boolean circuit family computes SAT (`SAT ∉ P/poly` → `P ≠ NP`).

Tools used:
- Boolean circuit model (AND/OR/NOT/MAJORITY gates, DAG structure)
- Switching lemma (Håstad 1987) - empirically verified
- Razborov-Rudich natural proofs audit - three-condition check
- Williams method (2011) - routing around the natural proofs barrier
- ACC⁰ lower bound structure

**Barrier routed around:** The natural proofs barrier (Razborov-Rudich 1994).

The Williams method avoids natural proofs by using an algorithmic
argument instead of a combinatorial property of function space.
It does not satisfy the `large` condition of Razborov-Rudich.

**New wall identified:** Algorithmic structure at P/poly.

The Williams method requires the target circuit class to have
exploitable structure. P/poly has no such structure by definition
— any exploitation would require already knowing P ≠ NP.

**Status:** `BARRIER ROUTED AROUND / NEW WALL IDENTIFIED`

The `NaturalProofAudit` struct checks each Razborov-Rudich condition
independently. The Williams decision procedure runs across ACC⁰,
TC⁰, and P/poly.

---

### Attempt 4 - [`theory4.c`](./theories/theory4.c) · [`theory4.md`](./theories/theory4.md)

**Strategy:** Second-order logic with full comprehension and
the categorical description of ℕ.

Key moves:
- SOL comprehension schema implemented and audited for predicativity
- Categorical ℕ via 7 arithmetic axioms + SOL induction axiom
- `|x|^k` defined internally - no external PA
- `PRelation(R)` and `NPRelation(R)` as genuine SOL formulas
- Diagonal via comprehension - new failure mode found
- **Categoricity route** - the genuinely new argument

**The categoricity argument:**

All full SOL models of the arithmetic + complexity axioms are
isomorphic to the standard model ℕ. The isomorphism preserves
`pow(|x|,k)` and machine indices. Therefore P vs NP has the same
truth value in all full models - it is **semantically determined**.

Oracle models (BGS) satisfy *different* complexity axioms — they
redefine polynomial time to include oracle calls. They are not
full SOL models of our axioms. **The relativization barrier does
not apply to the full SOL sentence.**

**New wall:** SOL incompleteness.

*"No effectively given derivation system is complete for the
full second-order semantics."*

The sentence is determined. The proof may not exist in any effective
system. This is stronger than ZFC independence.

**Status:** `DETERMINED IN FULL SOL / PROOF BLOCKED BY SOL INCOMPLETENESS`

---

### Forward Path - [`proof4.md`](./proofs/proof4.md)

The bridge document between Attempt 4 and Attempt 5.

Establishes three routes forward from the determination result,
selects Route 3 (descriptive complexity) as the most promising,
and derives the restatement:

```
P ≠ NP   iff   FO(LFP) ≠ ∃SO   on finite ordered structures
```

Documents why this route is non-relativizing (works on fixed
finite structures, not oracle worlds), non-natural (expressibility
arguments are not large combinatorial properties), and connects
directly to the SOL framework built in Attempt 4.

Also establishes the epistemic shift:

```
Before: "There may be no effective proof at all"
After:  "There is a concrete finite game whose outcome is the proof.
         We do not yet know how to win it."
```

---

### Attempt 5 - [`theory5.c`](./theories/theory5.c) · [`theory5.md`](./theories/theory5.md)

**Strategy:** Descriptive complexity. FO(LFP) vs ∃SO on finite
ordered structures. EF games and pebbling games as proof objects.

Implemented:
- Finite ordered graph structures with edge relation
- FO(LFP) with least fixed-point iteration (demonstrated on
  transitive closure - converges in ≤ n iterations on finite structures)
- ∃SO evaluator for 3-colorability (Fagin's theorem instance)
- Ehrenfeucht-Fraïssé game - FO lower bound confirmed
- Pebbling game - FO(LFP) lower bound attempted
- Three graph families: different degrees → same degree → CFI

**Evaluated structures:**

| Structure | 3-colorable? | ∃SO result |
|---|---|---|
| K4 | No | FALSE |
| K3 triangle | Yes | TRUE - witness [1,2,3] |
| C5 | Yes | TRUE - witness [1,2,1,2,3] |
| C6 bipartite | Yes | TRUE - witness [1,2,1,2,1,2] |

**The stall point — greedy coloring obstacle:**

FO(LFP) on ordered structures can compute the greedy chromatic
number (a valid LFP formula along the linear order `<`). But greedy
chromatic number is *order-dependent*. True 3-colorability is
*order-independent*. These do not coincide.

**The remaining obstacle - CFI-LFP:**

```
Construct A_n (3-colorable), B_n (not 3-colorable) such that
A_n ≡_{FO(LFP)} B_n  for all n large relative to k.
```

Cai-Fürer-Immerman (1992) solved this for FO. The extension to
FO(LFP) - the CFI-LFP problem - is open. Resolving it separates
P from NP.

**Five-step proof structure:**

```
(1) CFI-LFP construction         [OPEN]
(2) Duplicator strategy          [follows from 1]
(3) Order-independence argument  [partially known]
(4) LFP invariance under iteration [OPEN — hardest step]
(5) Extraction: P ≠ NP □         [follows from 1-4]
```

**Status:** `CFI-LFP PROBLEM ISOLATED — STEP (4) OPEN`

---

## Barrier Map

| Attempt | Strategy | Barrier | Outcome |
|---|---|---|---|
| 1 | Diagonalization | Relativization (BGS 1975) | Collapsed |
| 2 | FOL expression | Arithmetic + model-theoretic | Expression achieved |
| 3 | Circuit + Williams | Algorithmic structure at P/poly | Barrier routed around |
| 4 | SOL + categoricity | SOL incompleteness | Sentence determined |
| 5 | Descriptive complexity | CFI-LFP open problem | Obstacle isolated |
| 6 | CFI-LFP construction | TBD | In progress |

---

## Build and Run

Each theory file is a standalone C program. Compile with:

```bash
gcc -Wall -Wextra -o deduce   deduce.c
gcc -Wall -Wextra -o theory1  theory1.c
gcc -Wall -Wextra -o theory2  theory2.c
gcc -Wall -Wextra -o theory3  theory3.c -lm
gcc -Wall -Wextra -o theory4  theory4.c
gcc -Wall -Wextra -o theory5  theory5.c
```

Run any:

```bash
./theory5
```

All files compile clean with zero warnings on GCC and Clang.
Tested on Linux (Arch, Ubuntu 24).

---

## The Current Position

After five attempts, the series has converged on a single,
precisely-stated open problem:

> **Can we construct graphs `A_n` (3-colorable) and `B_n`
> (not 3-colorable) such that `A_n ≡_{FO(LFP)} B_n` — that
> no FO(LFP) sentence distinguishes them — and maintain a
> Duplicator bijection through all k rounds of the pebbling
> game `P_k(A_n, B_n)`, specifically through LFP iteration?**

That problem is P vs NP, restated without Turing machines, without
time bounds, without arithmetic, and without oracle relativization.
It is a finite combinatorial game. Its solution is the proof.

Attempt 6 attacks the CFI-LFP construction directly.

---

## About

**Swirly Crop** - Low-level systems engineering, GPU drivers,
WebGL, server infrastructure, and apparently computational logic.

This series is built in public as part of an ongoing research
effort into formal methods and complexity theory.

All C code compiles clean. All proofs are honest about where they fail.

---

*Swirly Crop · P vs NP Deductive Series*