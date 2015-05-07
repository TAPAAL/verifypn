For 2015 competition, there are two branches that were used:

lp:~verifypn-maintainers/verifypn/competition2015multiplePlaceBounds

that adds (using an ugly hack) the possibility to check for multiple
place bounds (in ReachabilityComputeBounds), otherwise it works as the trunk, and


lp:~verifypn-maintainers/verifypn/competition2015reachabilityBounds

that is used only for ReachabilityBounds (but does not support multiple
place bounds).

Both branches were modified inside the disk image by:
- adding SEQUENTIAL/COLLATERAL processing keywords
- for COLLATERAL processing outputing formula at the very end.

