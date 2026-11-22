/*
 * Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * This component and the accompanying materials are made available
 * under the terms of "Eclipse Public License v1.0"
 * which accompanies this distribution, and is available
 * at the URL "http://www.eclipse.org/legal/epl-v10.html".
 *
 * Initial Contributors:
 * Nokia Corporation - initial contribution.
 *
 * Contributors:
 *
 * Description:
 *
 */

package javax.microedition.m3g;

public class TriangleStripArray extends IndexBuffer {
	private int[] fkStripLengths;
	//------------------------------------------------------------------
	// Constructors
	//------------------------------------------------------------------

	public TriangleStripArray(int firstIndex, final int[] stripLengths) {
		super(_createImplicit(Interface.getHandle(),
				firstIndex,
				stripLengths));
		fkStoreStripLengths(stripLengths);
	}

	public TriangleStripArray(final int[] indices, final int[] stripLengths) {
		super(_createExplicit(Interface.getHandle(), indices, stripLengths));
		fkStoreStripLengths(stripLengths);
	}

	TriangleStripArray(long handle) {
		super(handle);
	}

	// M3G 1.1 Maintenance release getters

	public int getIndexCount() {
		return _getIndexCount(handle);
	}

	public void getIndices(int[] indices) {
		if (indices.length < _getIndexCount(handle)) {
			throw new IllegalArgumentException();
		}
		_getIndices(handle, indices);
	}

	/** Strip boundaries retained for compatibility adapters. */
	public int[] fkGetStripLengths() {
		if (fkStripLengths == null) return null;
		int[] copy = new int[fkStripLengths.length];
		System.arraycopy(fkStripLengths, 0, copy, 0, copy.length);
		return copy;
	}

	private void fkStoreStripLengths(int[] strips) {
		if (strips == null) return;
		fkStripLengths = new int[strips.length];
		System.arraycopy(strips, 0, fkStripLengths, 0, strips.length);
	}

	//------------------------------------------------------------------
	// Private methods
	//------------------------------------------------------------------

	// Native methods
	private native static long _createImplicit(long hInterface,
											  int first,
											  final int[] lengths);

	private native static long _createExplicit(long hInterface,
											  final int[] indices,
											  final int[] lengths);

	// M3G 1.1 Maintenance release getters
	private native static int _getIndexCount(long handle);

	private native static void _getIndices(long handle, int[] indices);

}
