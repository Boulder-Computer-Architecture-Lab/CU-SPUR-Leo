the variation of the attack is sa-ip.

since RAS / RSB units are indexed by thread in gem5, a cross-address space attack will require some work to do.

the transient.fail examples use threads to attempt out-of-place training, so it will face the same issue mentioned above. also, precise sleep functions on the scale used by the examples aren't implemented in SE mode, but one can write a crude substitute using the timer-related syscalls implemented.