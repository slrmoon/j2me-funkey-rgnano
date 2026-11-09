/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */


/*
 * Stub serial ports (builds without a serial port implementation, e.g. the
 * Win32 desktop build): every port fails to open.
 */

#include <serial_port_export.h>

int openPortByNameStart(char* pszDeviceName, int baudRate,
                        int options, int *pHandle, void **pContext) {
    (void)pszDeviceName; (void)baudRate; (void)options;
    (void)pHandle; (void)pContext;
    return PCSL_NET_IOERROR;
}

int openPortByNameFinish(char* pszDeviceName, int baudRate,
                         int options, int *pHandle, void *context) {
    (void)pszDeviceName; (void)baudRate; (void)options;
    (void)pHandle; (void)context;
    return PCSL_NET_IOERROR;
}

int configurePort(int hPort, int baudRate, unsigned int options) {
    (void)hPort; (void)baudRate; (void)options;
    return PCSL_NET_IOERROR;
}

int readFromPortStart(int hPort, char* pBuffer, int nNumberOfBytesToRead,
                      int* pBytesRead, void **pContext) {
    (void)hPort; (void)pBuffer; (void)nNumberOfBytesToRead;
    (void)pBytesRead; (void)pContext;
    return PCSL_NET_IOERROR;
}

int readFromPortFinish(int hPort, char* pBuffer, int nNumberOfBytesToRead,
                       int* pBytesRead, void *context) {
    (void)hPort; (void)pBuffer; (void)nNumberOfBytesToRead;
    (void)pBytesRead; (void)context;
    return PCSL_NET_IOERROR;
}

int writeToPortStart(int hPort, char* pBuffer, int nNumberOfBytesToWrite,
                     int* pBytesWritten, void **pContext) {
    (void)hPort; (void)pBuffer; (void)nNumberOfBytesToWrite;
    (void)pBytesWritten; (void)pContext;
    return PCSL_NET_IOERROR;
}

int writeToPortFinish(int hPort, char* pBuffer, int nNumberOfBytesToWrite,
                      int* pBytesWritten, void *context) {
    (void)hPort; (void)pBuffer; (void)nNumberOfBytesToWrite;
    (void)pBytesWritten; (void)context;
    return PCSL_NET_IOERROR;
}

void closePort(int hPort) {
    (void)hPort;
}
