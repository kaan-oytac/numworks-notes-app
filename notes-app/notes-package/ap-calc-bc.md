=== 1 Limits
# Limits and continuity
## Definition and rules
- $\lim_{x \to a} f(x) = L$: $f(x)$ gets as close to $L$ as we like when $x$ is close to $a$ (not at $a$).
- Limit exists only if left and right limits agree: $\lim_{x \to a^-} f = \lim_{x \to a^+} f$.
- Limits of sums, products, quotients (denominator $\ne 0$) and compositions follow the usual rules.
## Key limits
$$\lim_{x \to 0} \frac{\sin x}{x} = 1 \qquad \lim_{x \to 0} \frac{1 - \cos x}{x} = 0$$
$$\lim_{x \to \infty} \left(1 + \frac{1}{x}\right)^x = e \qquad \lim_{x \to 0} \frac{e^x - 1}{x} = 1$$
## Indeterminate forms
$\frac{0}{0}$, $\frac{\infty}{\infty}$, $0 \cdot \infty$, $\infty - \infty$, $1^\infty$, $0^0$, $\infty^0$. Try algebra first (factor, rationalize, common denominator), then L'Hopital.
## L'Hopital's rule
If $\frac{f(x)}{g(x)} \to \frac{0}{0}$ or $\frac{\infty}{\infty}$:
$$\lim \frac{f(x)}{g(x)} = \lim \frac{f'(x)}{g'(x)}$$
Rewrite other forms as a quotient first, e.g. $x \ln x = \frac{\ln x}{1/x}$.
## Continuity
$f$ is continuous at $a$ when $f(a)$ exists, $\lim_{x \to a} f(x)$ exists, and they are equal. Types of discontinuity: removable (hole), jump, infinite.
## Big theorems
- **IVT**: $f$ continuous on $[a,b]$ takes every value between $f(a)$ and $f(b)$.
- **EVT**: $f$ continuous on $[a,b]$ has an absolute max and min there.
- **Squeeze**: if $g \le f \le h$ and $g, h \to L$, then $f \to L$.
## Asymptotes
- Vertical: $\lim_{x \to a} f = \pm\infty$.
- Horizontal: $\lim_{x \to \pm\infty} f = L$. Compare growth: $\ln x$ is slower than $x^p$ is slower than $e^x$.

=== 2 Derivatives
# Derivatives
## Definition
$$f'(x) = \lim_{h \to 0} \frac{f(x+h) - f(x)}{h} = \lim_{t \to x} \frac{f(t) - f(x)}{t - x}$$
Differentiable implies continuous (not the reverse: corner, cusp, vertical tangent).
## Rules
- Power: $(x^n)' = n x^{n-1}$
- Product: $(uv)' = u'v + uv'$
- Quotient: $\left(\frac{u}{v}\right)' = \frac{u'v - uv'}{v^2}$
- Chain: $\frac{d}{dx} f(g(x)) = f'(g(x))\,g'(x)$
## Table
$$\frac{d}{dx} e^x = e^x \quad \frac{d}{dx} a^x = a^x \ln a \quad \frac{d}{dx} \ln x = \frac{1}{x}$$
$$\frac{d}{dx} \sin x = \cos x \quad \frac{d}{dx} \cos x = -\sin x \quad \frac{d}{dx} \tan x = \sec^2 x$$
$$\frac{d}{dx} \sec x = \sec x \tan x \quad \frac{d}{dx} \cot x = -\csc^2 x \quad \frac{d}{dx} \csc x = -\csc x \cot x$$
$$\frac{d}{dx} \arcsin x = \frac{1}{\sqrt{1-x^2}} \quad \frac{d}{dx} \arctan x = \frac{1}{1+x^2} \quad \frac{d}{dx} \text{arcsec}\,x = \frac{1}{|x|\sqrt{x^2-1}}$$
## Implicit and inverse
- Implicit: differentiate both sides, treat $y$ as $y(x)$, so $\frac{d}{dx} y^2 = 2y\,y'$, then solve for $y'$.
- Inverse function: if $g = f^{-1}$ then $g'(b) = \frac{1}{f'(g(b))}$.
- Logarithmic differentiation for $y = u^v$: $\ln y = v \ln u$, then differentiate.
## Tangent line and linearization
$$y = f(a) + f'(a)(x - a)$$
Overestimates where $f$ is concave down, underestimates where concave up.
## Related rates
1. Draw, name variables, write what is given and wanted (with units).
2. Find an equation linking the variables.
3. Differentiate with respect to $t$ (chain rule everywhere).
4. Substitute the instant's values last.

=== 3 Derivative applications
# Applications of derivatives
## Critical points and extrema
- Critical point: $f'(c) = 0$ or $f'(c)$ undefined (with $c$ in the domain).
- **First derivative test**: $f'$ changes $+$ to $-$ gives a local max; $-$ to $+$ gives a local min.
- **Second derivative test**: $f'(c) = 0$ and $f''(c) < 0$ gives a max; $f''(c) > 0$ gives a min; $f''(c) = 0$ is inconclusive.
- Absolute extrema on $[a,b]$: compare $f$ at critical points and at the endpoints.
## Concavity
- $f'' > 0$: concave up; $f'' < 0$: concave down.
- Inflection point: concavity changes ($f''$ changes sign), not just $f'' = 0$.
## Mean Value Theorem
If $f$ is continuous on $[a,b]$ and differentiable on $(a,b)$, some $c$ in $(a,b)$ has
$$f'(c) = \frac{f(b) - f(a)}{b - a}$$
Rolle's theorem is the case $f(a) = f(b)$, giving $f'(c) = 0$.
## Motion on a line
- $v = s'$, $a = v' = s''$. Speed is $|v|$.
- Speeding up when $v$ and $a$ have the same sign, slowing down when opposite.
- Displacement $= \int_a^b v\,dt$; distance $= \int_a^b |v|\,dt$.
## Optimization
Write the quantity to optimize in one variable using the constraint, find critical points, check endpoints and the domain, and answer the actual question (with units).
## Justifications the graders want
- "$f$ has a relative max at $x = c$ because $f'$ changes from positive to negative at $c$."
- "$f$ is increasing on the interval because $f' > 0$ there."
- "Point of inflection because $f''$ changes sign."

=== 4 Integrals
# Integrals
## Definitions
Riemann sum with $n$ subintervals, $\Delta x = \frac{b-a}{n}$, $x_k = a + k\Delta x$:
$$\int_a^b f(x)\,dx = \lim_{n \to \infty} \sum_{k=1}^{n} f(x_k)\,\Delta x$$
Left, right, midpoint sums; trapezoid $= \frac{\Delta x}{2}\left(f_0 + 2f_1 + \ldots + 2f_{n-1} + f_n\right)$ for equal widths. Increasing $f$: left under, right over. Concave up: trapezoid over, midpoint under.
## Fundamental Theorem of Calculus
$$\int_a^b f(x)\,dx = F(b) - F(a) \quad \text{where } F' = f$$
$$\frac{d}{dx} \int_a^{x} f(t)\,dt = f(x) \qquad \frac{d}{dx} \int_a^{g(x)} f(t)\,dt = f(g(x))\,g'(x)$$
## Properties
$\int_a^a f = 0$, $\int_b^a f = -\int_a^b f$, $\int_a^b f = \int_a^c f + \int_c^b f$, constants factor out.
## Antiderivative table
$$\int x^n\,dx = \frac{x^{n+1}}{n+1} + C \ (n \ne -1) \qquad \int \frac{1}{x}\,dx = \ln|x| + C$$
$$\int e^x\,dx = e^x + C \qquad \int a^x\,dx = \frac{a^x}{\ln a} + C$$
$$\int \sin x\,dx = -\cos x + C \qquad \int \cos x\,dx = \sin x + C$$
$$\int \sec^2 x\,dx = \tan x + C \qquad \int \sec x \tan x\,dx = \sec x + C$$
$$\int \tan x\,dx = -\ln|\cos x| + C \qquad \int \sec x\,dx = \ln|\sec x + \tan x| + C$$
$$\int \frac{dx}{\sqrt{a^2 - x^2}} = \arcsin\frac{x}{a} + C \qquad \int \frac{dx}{a^2 + x^2} = \frac{1}{a}\arctan\frac{x}{a} + C$$
## Techniques (BC)
- **u-substitution**: $u = g(x)$, $du = g'(x)\,dx$; change the limits too for definite integrals.
- **Integration by parts**: $\int u\,dv = uv - \int v\,du$. Choose $u$ by LIATE (log, inverse trig, algebraic, trig, exponential).
- **Partial fractions** (linear, non-repeated factors): $\frac{px+q}{(x-a)(x-b)} = \frac{A}{x-a} + \frac{B}{x-b}$.
- **Improper integrals**: replace the bad limit by a variable and take a limit, e.g. $\int_1^\infty \frac{dx}{x^p}$ converges only for $p > 1$.
## Average value
$$f_{avg} = \frac{1}{b-a} \int_a^b f(x)\,dx$$
## Net change
$$F(b) = F(a) + \int_a^b F'(t)\,dt$$

=== 5 Integral applications
# Applications of integrals
## Area
- Between curves: $\int_a^b \left(\text{top} - \text{bottom}\right)dx$ or $\int_c^d \left(\text{right} - \text{left}\right)dy$.
- Split the integral where the curves cross.
## Volumes
- **Disk** (rotating about the x-axis): $V = \pi \int_a^b r(x)^2\,dx$
- **Washer**: $V = \pi \int_a^b \left(R^2 - r^2\right)dx$ with $R$ the outer radius.
- Axis $y = k$: radius is $|f(x) - k|$.
- **Cross sections** on a base: $V = \int_a^b A(x)\,dx$. Squares: $A = s^2$; semicircles: $A = \frac{\pi}{8} s^2$; equilateral triangles: $A = \frac{\sqrt{3}}{4} s^2$.
- **Shells** (BC, optional): $V = 2\pi \int_a^b (\text{radius})(\text{height})\,dx$.
## Arc length (BC)
$$L = \int_a^b \sqrt{1 + f'(x)^2}\,dx$$
## Accumulation
If $R(t)$ is a rate (in units per time), $\int_a^b R(t)\,dt$ is the total change from $t = a$ to $t = b$. Amount at time $b$ is amount at $a$ plus that integral.

=== 6 Differential equations
# Differential equations
## Separable equations
$$\frac{dy}{dx} = g(x)\,h(y) \;\to\; \int \frac{dy}{h(y)} = \int g(x)\,dx$$
Solve for $y$, use the initial condition to find $C$ (do this before or after solving, but keep the domain in mind).
## Exponential growth and decay
$$\frac{dy}{dt} = k y \;\to\; y = y_0 e^{kt}$$
## Slope fields
Each tiny segment at $(x, y)$ has slope $\frac{dy}{dx}$ there. Sketch solutions by following the segments; check where the slope is zero or undefined.
## Euler's method (BC)
$$y_{n+1} = y_n + h \cdot f(x_n, y_n) \qquad x_{n+1} = x_n + h$$
Tabulate $x$, $y$, slope, $\Delta y$. Concave up solutions make Euler underestimate.
## Logistic growth (BC)
$$\frac{dP}{dt} = kP\left(1 - \frac{P}{L}\right) \qquad P(t) = \frac{L}{1 + A e^{-kt}}$$
- $L$ is the carrying capacity: $\lim_{t \to \infty} P = L$.
- Fastest growth (inflection point) when $P = \frac{L}{2}$.

=== 7 Parametric polar vector
# Parametric, polar, vector (BC)
## Parametric curves
$$\frac{dy}{dx} = \frac{dy/dt}{dx/dt} \qquad \frac{d^2y}{dx^2} = \frac{\frac{d}{dt}\left(\frac{dy}{dx}\right)}{dx/dt}$$
$$L = \int_a^b \sqrt{\left(\frac{dx}{dt}\right)^2 + \left(\frac{dy}{dt}\right)^2}\,dt$$
Horizontal tangent: $\frac{dy}{dt} = 0$ (and $\frac{dx}{dt} \ne 0$). Vertical tangent: $\frac{dx}{dt} = 0$.
## Vector-valued motion
- Position $\langle x(t), y(t) \rangle$, velocity $\langle x'(t), y'(t) \rangle$, acceleration $\langle x''(t), y''(t) \rangle$.
- Speed $= \sqrt{x'(t)^2 + y'(t)^2}$; distance traveled $= \int_a^b \text{speed}\,dt$.
- Position at $t = b$: $x(b) = x(a) + \int_a^b x'(t)\,dt$, same for $y$.
## Polar coordinates
$$x = r\cos\theta \qquad y = r\sin\theta \qquad r^2 = x^2 + y^2 \qquad \tan\theta = \frac{y}{x}$$
$$\text{Area} = \frac{1}{2} \int_\alpha^\beta r^2\,d\theta$$
$$\frac{dy}{dx} = \frac{dy/d\theta}{dx/d\theta} = \frac{r'\sin\theta + r\cos\theta}{r'\cos\theta - r\sin\theta}$$
- Area between curves: $\frac{1}{2}\int (r_{out}^2 - r_{in}^2)\,d\theta$; find intersections first.
- $\frac{dr}{d\theta} > 0$ means the point moves away from the origin.

=== 8 Series
# Sequences and series (BC)
## Basics
- $\sum a_n$ converges if its partial sums $S_n$ have a limit.
- **nth term test**: if $a_n$ does not tend to $0$, the series diverges. (If $a_n \to 0$ you know nothing.)
- Geometric: $\sum_{n=0}^{\infty} a r^n = \frac{a}{1 - r}$ when $|r| < 1$, diverges otherwise.
- p-series: $\sum \frac{1}{n^p}$ converges only for $p > 1$. Harmonic ($p = 1$) diverges.
- Telescoping: write partial sums and cancel.
## Convergence tests
- **Integral test**: $f$ positive, continuous, decreasing; $\sum f(n)$ and $\int_1^\infty f$ behave the same.
- **Comparison**: $0 \le a_n \le b_n$; $\sum b_n$ converges gives $\sum a_n$ converges; $\sum a_n$ diverges gives $\sum b_n$ diverges.
- **Limit comparison**: $\lim \frac{a_n}{b_n} = c$ with $0 < c < \infty$, then both behave the same.
- **Alternating series**: $b_n$ decreasing and $b_n \to 0$ gives convergence of $\sum (-1)^n b_n$. Error bound: $|S - S_n| \le b_{n+1}$.
- **Ratio test**: $\lim \left|\frac{a_{n+1}}{a_n}\right| = \rho$. Converges absolutely if $\rho < 1$, diverges if $\rho > 1$, inconclusive if $\rho = 1$.
- Absolute convergence ($\sum |a_n|$ converges) implies convergence; conditional means $\sum a_n$ converges but $\sum |a_n|$ does not.
## Quick reference
| Series | Converges when |
|:-------|:---------------|
| $\sum ar^n$ | $|r| < 1$ |
| $\sum \frac{1}{n^p}$ | $p > 1$ |
| $\sum (-1)^n b_n$ | $b_n$ decreasing, $b_n \to 0$ |
| Ratio test | $\rho < 1$ |
| Integral test | $\int_1^\infty f$ converges |
## Power series
$$\sum_{n=0}^{\infty} c_n (x - a)^n$$
Use the ratio test to find the radius $R$; test the two endpoints separately for the interval of convergence. Differentiate or integrate term by term inside the interval (same radius).
## Taylor and Maclaurin series
$$f(x) = \sum_{n=0}^{\infty} \frac{f^{(n)}(a)}{n!}(x - a)^n$$
$$e^x = \sum_{n=0}^{\infty} \frac{x^n}{n!} = 1 + x + \frac{x^2}{2!} + \frac{x^3}{3!} + \ldots$$
$$\sin x = \sum_{n=0}^{\infty} \frac{(-1)^n x^{2n+1}}{(2n+1)!} = x - \frac{x^3}{3!} + \frac{x^5}{5!} - \ldots$$
$$\cos x = \sum_{n=0}^{\infty} \frac{(-1)^n x^{2n}}{(2n)!} = 1 - \frac{x^2}{2!} + \frac{x^4}{4!} - \ldots$$
$$\frac{1}{1-x} = \sum_{n=0}^{\infty} x^n \quad (|x| < 1) \qquad \ln(1+x) = \sum_{n=1}^{\infty} \frac{(-1)^{n+1} x^n}{n} \quad (-1 < x \le 1)$$
$$\arctan x = \sum_{n=0}^{\infty} \frac{(-1)^n x^{2n+1}}{2n+1} \quad (|x| \le 1)$$
Build new series by substitution ($e^{-x^2}$), multiplication by $x^k$, differentiation and integration.
## Lagrange error bound
$$|R_n(x)| \le \frac{M}{(n+1)!}|x - a|^{n+1} \quad \text{with } M \ge \left|f^{(n+1)}(z)\right| \text{ between } a \text{ and } x$$
For alternating series use the alternating series bound instead: the error is at most the first omitted term.

=== 9 Exam tips
# Exam reminders
- Units on every final answer in applied problems; three decimal places unless told otherwise.
- Show the setup: write the integral or derivative before using the calculator.
- Calculator sections: store intermediate values, do not round early.
- Justify with the right object: increasing uses $f'$, concavity and inflection use $f''$, extrema use sign changes.
- Read the given information: a table means use secant slopes, Riemann sums or MVT; a graph of $f'$ is not a graph of $f$.
- When an integral of a rate is asked, say what it means in context ("the total amount of water that entered the tank between...").
- Check the domain after solving a differential equation and keep the branch that contains the initial condition.
