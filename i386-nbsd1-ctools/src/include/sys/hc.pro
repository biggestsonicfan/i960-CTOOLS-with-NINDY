/*
 *   Pragma definitions required for gnu960 tools to
 *   compile with the Metaware compiler. This "profile" file
 *   is used instead of command-line options to workaround
 *   the 128-char command line limit on DOS.
 */
#pragma On(Check_stack);
#pragma On(Multiple_var_defs);
#pragma Off(Char_default_unsigned);
#pragma On(PCC)
#pragma Off(Floating_point)
