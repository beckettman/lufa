/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "ConfigDescriptor.h"

uint8_t PrinterInterfaceNumber;
uint8_t PrinterAltSetting;


uint8_t ProcessConfigurationDescriptor(void)
{
	uint8_t* ConfigDescriptorData;
	uint16_t ConfigDescriptorSize;
	uint8_t  ErrorCode;
	uint8_t  FoundEndpoints = 0;
	
	/* Get Configuration Descriptor size from the device */
	if (USB_GetDeviceConfigDescriptor(1, &ConfigDescriptorSize, NULL) != HOST_SENDCONTROL_Successful)
	  return ControlError;
	
	/* Ensure that the Configuration Descriptor isn't too large */
	if (ConfigDescriptorSize > MAX_CONFIG_DESCRIPTOR_SIZE)
	  return DescriptorTooLarge;
	  
	/* Allocate enough memory for the entire config descriptor */
	ConfigDescriptorData = alloca(ConfigDescriptorSize);

	/* Retrieve the entire configuration descriptor into the allocated buffer */
	USB_GetDeviceConfigDescriptor(1, &ConfigDescriptorSize, ConfigDescriptorData);
	
	/* Validate returned data - ensure first entry is a configuration header descriptor */
	if (DESCRIPTOR_TYPE(ConfigDescriptorData) != DTYPE_Configuration)
	  return InvalidConfigDataReturned;
	
	/* Get the printer interface from the configuration descriptor */
	if ((ErrorCode = USB_GetNextDescriptorComp(&ConfigDescriptorSize, &ConfigDescriptorData,
	                                           NextBidirectionalPrinterInterface)))
	{
		/* Descriptor not found, error out */
		return NoInterfaceFound;
	}
	
	PrinterInterfaceNumber = DESCRIPTOR_CAST(ConfigDescriptorData, USB_Descriptor_Interface_t).InterfaceNumber;
	PrinterAltSetting      = DESCRIPTOR_CAST(ConfigDescriptorData, USB_Descriptor_Interface_t).AlternateSetting;

	/* Get the IN and OUT data endpoints for the mass storage interface */
	while (FoundEndpoints != ((1 << PRINTER_DATA_OUT_PIPE) | (1 << PRINTER_DATA_IN_PIPE)))
	{
		/* Fetch the next bulk endpoint from the current printer interface */
		if ((ErrorCode = USB_GetNextDescriptorComp(&ConfigDescriptorSize, &ConfigDescriptorData,
		                                           NextInterfaceBulkDataEndpoint)))
		{
			/* Descriptor not found, error out */
			return NoEndpointFound;
		}
		
		USB_Descriptor_Endpoint_t* EndpointData = DESCRIPTOR_PCAST(ConfigDescriptorData, USB_Descriptor_Endpoint_t);

		/* Check if the endpoint is a bulk IN or bulk OUT endpoint, set appropriate globals */
		if (EndpointData->EndpointAddress & ENDPOINT_DESCRIPTOR_DIR_IN)
		{
			/* Configure the data IN pipe */
			Pipe_ConfigurePipe(PRINTER_DATA_IN_PIPE, EP_TYPE_BULK, PIPE_TOKEN_IN,
			                   EndpointData->EndpointAddress, EndpointData->EndpointSize,
			                   PIPE_BANK_SINGLE);

			Pipe_SetInfiniteINRequests();

			/* Set the flag indicating that the data IN pipe has been found */
			FoundEndpoints |= (1 << PRINTER_DATA_IN_PIPE);
		}
		else
		{
			/* Configure the data OUT pipe */
			Pipe_ConfigurePipe(PRINTER_DATA_OUT_PIPE, EP_TYPE_BULK, PIPE_TOKEN_OUT,
			                   EndpointData->EndpointAddress, EndpointData->EndpointSize,
			                   PIPE_BANK_SINGLE);

			/* Set the flag indicating that the data OUT pipe has been found */
			FoundEndpoints |= (1 << PRINTER_DATA_OUT_PIPE);
		}		
	}

	/* Valid data found, return success */
	return SuccessfulConfigRead;
}

uint8_t NextBidirectionalPrinterInterface(void* CurrentDescriptor)
{
	/* PURPOSE: Find next Bidirectional protocol printer class interface descriptor */

	if (DESCRIPTOR_TYPE(CurrentDescriptor) == DTYPE_Interface)
	{
		/* Check the descriptor class and protocol, break out if correct class/protocol interface found */
		if ((DESCRIPTOR_CAST(CurrentDescriptor, USB_Descriptor_Interface_t).Class    == PRINTER_CLASS)    &&
		    (DESCRIPTOR_CAST(CurrentDescriptor, USB_Descriptor_Interface_t).SubClass == PRINTER_SUBCLASS) &&
			(DESCRIPTOR_CAST(CurrentDescriptor, USB_Descriptor_Interface_t).Protocol == PROTOCOL_BIDIRECTIONAL))
		{
			return DESCRIPTOR_SEARCH_Found;
		}
	}
	
	return DESCRIPTOR_SEARCH_NotFound;
}

uint8_t NextInterfaceBulkDataEndpoint(void* CurrentDescriptor)
{
	/* PURPOSE: Find next interface bulk endpoint descriptor before next interface descriptor */

	if (DESCRIPTOR_TYPE(CurrentDescriptor) == DTYPE_Endpoint)
	{
		uint8_t EndpointType = (DESCRIPTOR_CAST(CurrentDescriptor,
		                                        USB_Descriptor_Endpoint_t).Attributes & EP_TYPE_MASK);

		/* Check the endpoint type, break out if correct BULK type endpoint found */
		if (EndpointType == EP_TYPE_BULK)
		  return DESCRIPTOR_SEARCH_Found;
	}
	else if (DESCRIPTOR_TYPE(CurrentDescriptor) == DTYPE_Interface)
	{
		return DESCRIPTOR_SEARCH_Fail;
	}

	return DESCRIPTOR_SEARCH_NotFound;
}
