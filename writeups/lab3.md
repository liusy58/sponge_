Lab 3 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

I worked with or talked about this assignment with: [please list other sunetids]

Program Structure and Design of the TCPSender:
[]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

I think the only way to fully understand TCP is to materialize the state variable.



resend:
* TCPSegments and deciding if the oldest-sent segment has been outstanding for too long
  without acknowledgment (that is, without all of its sequence numbers being acknowledged).
  If so, it needs to be retransmitted (sent again).
  
* 
 
