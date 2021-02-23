#ifndef __DELIVERYOPTIMIZATION_ERROR_H__
#define __DELIVERYOPTIMIZATION_ERROR_H__

//  Definitions of DeliveryOptimization error codes
//
//  Error codes are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//      Sev         - is the severity code
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//      C           - is the Customer code flag
//      R           - is a reserved bit
//      Facility    - is the facility code
//      Code        - is the facility's status code
//
//  Severity used is 2 (warning). This was chosen to make the codes similar (start with 0x80) to other components like WU.
//  Facility code used is 208 (0xD0). See winerror.h for FACILITY_DELIVERY_OPTIMIZATION.
//  Note that the uniqueness of facility codes isn't really enforced in Windows.
//
//  Error codes used to be separated into zones with a macro identifying each zone.
//  Currently, we only care about the transient error zone so only this macro is defined.
//  The error codes are still separated with sufficient buffer left to add errors in each zone.

#define DO_ZONE_MASK               0xF800
#define DO_TRANSIENT_ZONE          0x3800

#define DO_E_NO_SERVICE                  HRESULT(0x80D01001L)   // Delivery Optimization was unable to provide the service

// Download job codes

#define DO_E_DOWNLOAD_NO_PROGRESS                   HRESULT(0x80D02002L)    // Download of a file saw no progress within the defined period
#define DO_E_UNKNOWN_PROPERTY_ID                    HRESULT(0x80D02011L)    // SetProperty() or GetProperty() called with an unknown property ID
#define DO_E_READ_ONLY_PROPERTY                     HRESULT(0x80D02012L)    // Unable to call SetProperty() on a read-only property
#define DO_E_INVALID_STATE                          HRESULT(0x80D02013L)    // The requested action is not allowed in the current job state. The job might have been canceled or completed transferring. It is in a read-only state now.
#define DO_E_FILE_DOWNLOADSINK_UNSPECIFIED          HRESULT(0x80D02018L)    // Unable to start a download because no download sink (either local file or stream interface) was specified

// IDODownload interface

#define DO_E_DOWNLOAD_NO_URI             HRESULT(0x80D02200L)   // The download was started without providing a URI

// Transient conditions

#define DO_E_BLOCKED_BY_NO_NETWORK      HRESULT(0x80D03805L)    // Download paused due to loss of network connectivity

#endif // __DELIVERYOPTIMIZATION_ERROR_H__
