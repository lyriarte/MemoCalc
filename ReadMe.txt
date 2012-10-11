MemoCalc v0.7

- Lexer and Parser functional. Uses MathLib. Evaluation with +-*/ operators only if MathLib not present.
- UI with variables and functions lists, memo editor, FlpCompDblToA. Saves last memo index in a feature.
- Added constants e, pi, g, c. Variables with the same name override constants. Hide button '=' when not editing variables.
- Added a scrollbar for the MemoEdit view. Use DmDeleteRecord instead of DmRemoveRecord. Save last memo uniqueID instead of index. Enable PgUp PgDown in edit view.
- Code formatting, bugfixes. Fix evaluation of expressions like "a-b-c-d". Fix fatal alert at "Quit and discard changes".
- Removed checks on form focus, which failed on Treo600. Add a "Keyboard" menu in the edit view.