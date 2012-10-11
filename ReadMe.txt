MemoCalc v0.9

- Lexer and Parser functional. Uses MathLib. Evaluation with +-*/ operators only if MathLib not present.
- UI with variables and functions lists, memo editor, FlpCompDblToA. Saves last memo index in a feature.
- Added constants e, pi, g, c. Variables with the same name override constants. Hide button '=' when not editing variables.
- Added a scrollbar for the MemoEdit view. Use DmDeleteRecord instead of DmRemoveRecord. Save last memo uniqueID instead of index. Enable PgUp PgDown in edit view.
- Code formatting, bugfixes. Fix evaluation of expressions like "a-b-c-d". Fix fatal alert at "Quit and discard changes".
- Removed checks on form focus, which failed on Treo600. Add a "Keyboard" menu in the edit view.
- Changed application name from "Memo Calc" to "MemoCalc" to avoid confusion when installed from a card reader. DELETE THE PREVIOUS VERSION BEFORE INSTALLING THIS ONE. Changed version string resource ID to 1000 so it actually shows up in Launcher. Fixed error management for calulation error, division by zero, sqrt negative etc...
- Fixed a bug with 0.8 that caused evaluation to fail when MathLib was not present