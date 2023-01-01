# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(bf-hit-rate) begin
(bf-hit-rate) create "big"
(bf-hit-rate) create "small"
(bf-hit-rate) open "small"
(bf-hit-rate) open "big"
(bf-hit-rate) open "small"
(bf-hit-rate) open "small"
(bf-hit-rate) cache ok
(bf-hit-rate) end
EOF
pass;
