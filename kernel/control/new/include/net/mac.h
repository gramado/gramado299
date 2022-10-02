/*
 * mac.h
 *     MAC address info manager.
 * 2016.
 */


// J� tem um arquivo com esse nome em net/
#ifndef ____HAL_MAC_H
#define ____HAL_MAC_H    1


struct mac_address_d
{
    object_type_t  objectType;
    object_class_t objectClass;

    //6 bytes. (3 de uma vari�vel e tres de outra.)
    unsigned long mac_address_high;   //OUI
    unsigned long mac_address_low;    //NIC

};

struct mac_address_d  *mac_address;
//struct mac_address_d  mac_address
//...


#endif   



//
// End.
//




