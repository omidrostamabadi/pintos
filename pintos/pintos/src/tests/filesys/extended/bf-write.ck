# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(bf-write) begin
(bf-write) create "big"
(bf-write) open "big"
(bf-write) cache ok
(bf-write) end
EOF
pass;