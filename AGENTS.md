# Required validation for future VR changes

For every mod update, review hot-path loops, blocking calls and allocations for frame stalls. Keep scene scans and tool contact probes bounded. Run the build, stereo/input/contact tests and applicable OpenGL/performance-profile tests. Run tests/check_freeze_log.ps1 against the latest session and record which revision produced that log. A previous revision's log is not validation of a new DLL.

Never claim freezing is fixed from compilation or offline tests. In-game performance needs a fresh headset session including loading, tool switching, both-hand grabs, throwing and opening/closing the B menu. Report any remaining stalls and missing runtime validation clearly.
