# HTTP Client V2

This a copy of the HTTP_client library with added functionality for HTTP chunked transfers.

The support for chunked transfers is partial and will work only if a single chunk is 
being sent by the server. The code will also not support HTTP extensions for chunked transfers. 

The code currently only modifies the deferred_receive_handler function,
and in particular only the section where HTTP_HEADER_CHUNKED is found in the header, 
which is empty in the 6.6.0 release. If you wish to upgrade to a newer library release
you may be able to just modify this function section.