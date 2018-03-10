.. |br| raw:: html

   <br />

Libmodule Concepts
==================

Module concept
--------------

A module is key entity of libmodule: it is a single and indipendent logical unit that reacts to certain events by polling on a fd. |br|
It offers some callbacks that are used by libmodule to manage its life. |br|
It is initialized through MODULE macro:
   
.. code::
    
    MODULE(test)
    
This macro creates a "test" module. |br|
MODULE macro also creates a constructor and destructor that are automatically called by libmodule at start and at end of program. |br|
Finally, this macro declares all of needed callbacks and returns an opaque handler for the module, that will be transparently passed with each call to libmodule API while using easy AP:ref:`module_easy`. |br|

Submodule concept
-----------------

PLACEHOLDER

Context concept
---------------

PLACEHOLDER
