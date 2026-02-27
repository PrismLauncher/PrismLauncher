# Contributions Guidelines

## Restrictions on Generative AI Usage (AI Policy)

> [!NOTE]
> The following is adapted from [matplotlib's contributing guide](https://matplotlib.org/devdocs/devel/contribute.html#generative-ai) and the [Linux Kernel policy guide](https://www.kernel.org/doc./html/next/process/coding-assistants.html)

We expect authentic engagement in our community.

- Do not post output from Large Language Models or similar generative AI as comments on GitHub or our discord server, as such comments tend to be formulaic and low-quality content.
- If you use generative AI tools as an aid in developing code or documentation changes, ensure that you fully understand the proposed changes and can explain why they are the correct approach.

Make sure you have added value based on your personal competency to your contributions.
Just taking some input, feeding it to an AI and posting the result is not of value to the project.
To preserve precious core developer capacity, we reserve the right to rigorously reject seemingly AI generated low-value contributions.

### Signed-off-by and Developer Certificate of Origin

AI agents MUST NOT add Signed-off-by tags. Only humans can legally certify the Developer Certificate of Origin (DCO). The human submitter is responsible for:

- Reviewing all AI-generated code
- Ensuring compliance with licensing requirements
- Adding their own Signed-off-by tag to certify the DCO
- Taking full responsibility for the contribution

See [Signing your work](#signing-your-work) for more information.

### Attribution

When AI tools contribute to development, proper attribution helps track the evolving role of AI in the development process. Contributions should include an Assisted-by tag in the commit message with the following format:

```text
Assisted-by: AGENT_NAME:MODEL_VERSION [TOOL1] [TOOL2]
```

Where:

- `AGENT_NAME` is the name of the AI tool or framework
- `MODEL_VERSION` is the specific model version used
- `[TOOL1] [TOOL2]` are optional specialized analysis tools used (e.g., coccinelle, sparse, smatch, clang-tidy)

Basic development tools (git, gcc, make, editors) should not be listed.

Example:

```text
Assisted-by: Claude:claude-3-opus coccinelle sparse
```

## Code style

All files are formatted with `clang-format` using the configuration in `.clang-format`. Ensure it is run on changed files before committing!

Please also follow the project's conventions for C++:

- Class and type names should be formatted as `PascalCase`: `MyClass`.
- Private or protected class data members should be formatted as `camelCase` prefixed with `m_`: `m_myCounter`.
- Private or protected `static` class data members should be formatted as `camelCase` prefixed with `s_`: `s_instance`.
- Public class data members should be formatted as `camelCase` without the prefix: `dateOfBirth`.
- Public, private or protected `static const` class data members should be formatted as `SCREAMING_SNAKE_CASE`: `MAX_VALUE`.
- Class function members should be formatted as `camelCase` without a prefix: `incrementCounter`.
- Global functions and non-`const` global variables should be formatted as `camelCase` without a prefix: `globalData`.
- `const` global variables and macros should be formatted as `SCREAMING_SNAKE_CASE`: `LIGHT_GRAY`.
- enum constants should be formatted as `PascalCase`: `CamelusBactrianus`
- Avoid inventing acronyms or abbreviations especially for a name of multiple words - like `tp` for `texturePack`.
- Avoid using `[[nodiscard]]` unless ignoring the return value is likely to cause a bug in cases such as:
  - A function allocates memory or another resource and the caller needs to clean it up.
  - A function has side effects and an error status is returned.
  - A function is likely be mistaken for having side effects.
- A plain getter is unlikely to cause confusion and adding `[[nodiscard]]` can create clutter and inconsistency.

Most of these rules are included in the `.clang-tidy` file, so you can run `clang-tidy` to check for any violations.

Here is what these conventions with the formatting configuration look like:

```c++
#define AWESOMENESS 10

constexpr double PI = 3.14159;

enum class PizzaToppings { HamAndPineapple, OreoAndKetchup };

struct Person {
    QString name;
    QDateTime dateOfBirth;

    long daysOld() const { return dateOfBirth.daysTo(QDateTime::currentDateTime()); }
};

class ImportantClass {
   public:
    void incrementCounter()
    {
        if (m_counter + 1 > MAX_COUNTER_VALUE)
            throw std::runtime_error("Counter has reached limit!");

        ++m_counter;
    }

    int counter() const { return m_counter; }

   private:
    static constexpr int MAX_COUNTER_VALUE = 100;
    int m_counter;
};

ImportantClass importantClassInstance;
```

If you see any names which do not follow these conventions, it is preferred that you leave them be - renames increase the number of changes therefore make reviewing harder and make your PR more prone to conflicts. However, if you're refactoring a whole class anyway, it's fine.

## Signing your work

In an effort to ensure that the code you contribute is actually compatible with the licenses in this codebase, we require you to sign-off all your contributions.

This can be done by appending `-s` to your `git commit` call, or by manually appending the following text to your commit message:

```text
<commit message>

Signed-off-by: Author name <Author email>
```

By signing off your work, you agree to the terms below:

```text
Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

These terms will be enforced once you create a pull request, and you will be informed automatically if any of your commits aren't signed-off by you.

As a bonus, you can also [cryptographically sign your commits][gh-signing-commits] and enable [vigilant mode][gh-vigilant-mode] on GitHub.

[gh-signing-commits]: https://docs.github.com/en/authentication/managing-commit-signature-verification/signing-commits
[gh-vigilant-mode]: https://docs.github.com/en/authentication/managing-commit-signature-verification/displaying-verification-statuses-for-all-of-your-commits

## Backporting to Release Branches

We use [automated backports](https://github.com/PrismLauncher/PrismLauncher/blob/develop/.github/workflows/backport.yml) to merge specific contributions from develop into `release` branches.

This is done when pull requests are merged and have labels such as `backport release-7.x` - which should be added along with the milestone for the release.
