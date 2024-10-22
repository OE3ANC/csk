# Benchmark demo


Overview
********

The benchmark demo tests the performance in various cases. 
For example rectangle, border, shadow, text, image blending, image transformation, blending modes, etc. 
All tests are repeated with 50% opacity.

The size and position of the objects during testing are set with a pseudo random number to make the benchmark repeatable. 

On to top of the screen the title of the current test step, and the result of the previous step is displayed.

Building and Running
********************

Make sure you have the CSK6 connected over USB port.

The sample can be built and flashed as follows:

.. code-block:: console

   west build -b csk6 samples/subsys/display/lvgl/benchmark
   west flash

Sample Output 
=============

*** Booting Zephyr OS build v1.0.3-alpha.1  ***
Requirements
************

This sample use LCD ili9488 to demonstrate lvgl8 benchmark