# Values differ on every machine; keep only the labels.
s/^  ([^:]+):.*/  \1: X/
# The swap line only appears on machines that have swap.
/^  Takas alanı:/d
