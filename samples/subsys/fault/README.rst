fault Sample Application
##########################

Overview
********

This sample is mainly used to test and demonstrate different types of runtime errors, such as division by zero, stack overflow, and null pointer dereference. This is useful for understanding and learning error handling in the operating system.

Building and Running
********************

Make sure you have the CSK6 hard fault sample

The sample can be built and flashed as follows:

.. code-block:: console

   west build -b csk6 samples/subsys/fault/fault
   west flash
