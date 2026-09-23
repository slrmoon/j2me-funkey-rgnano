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

public class Transform {
	//------------------------------------------------------------------
	// Static data
	//------------------------------------------------------------------
	private static final int HISTORY_SIZE = 16;
	private static final int MAX_DUMPS = 4;
	private static int nextTraceId;
	private static int dumpCount;
	private static int[] dumpedIds = new int[MAX_DUMPS];
	private static final boolean TRACE_VERBOSE = _traceVerbose();

	//------------------------------------------------------------------
	// Instance data
	//------------------------------------------------------------------

	// First 72 bytes match m3g_math.h Matrix; last 4 carry the trace ID.
	byte[] matrix = new byte[76];
	private final int traceId;
	private byte[] historyOps = new byte[HISTORY_SIZE];
	private float[] historyBefore = new float[HISTORY_SIZE * 16];
	private float[] historyAfter = new float[HISTORY_SIZE * 16];
	private float[] traceBefore = new float[16];
	private float[] traceAfter = new float[16];
	private int historyCount;

	//------------------------------------------------------------------
	// Constructor(s)
	//------------------------------------------------------------------

	public Transform() {
		if (!Platform.uiThreadAvailable()) {
			throw new Error("UI thread not initialized");
		}
		traceId = ++nextTraceId;
		_setTraceId(matrix, traceId);
		setIdentity();
	}

	/**
	 */
	public Transform(Transform other) {
		if (!Platform.uiThreadAvailable()) {
			throw new Error("UI thread not initialized");
		}
		traceId = ++nextTraceId;
		_setTraceId(matrix, traceId);
		set(other);
	}

	//------------------------------------------------------------------
	// Public methods
	//------------------------------------------------------------------

	public void setIdentity() {
		traceBegin(1);
		_setIdentity(matrix);
		traceEnd(1);
	}

	public void set(Transform transform) {
		traceBegin(2);
		System.arraycopy(transform.matrix, 0,
				this.matrix, 0,
				this.matrix.length);
		_setTraceId(matrix, traceId);
		traceEnd(2);
	}

	public void set(float[] matrix) {
		traceBegin(3);
		_setMatrix(this.matrix, matrix);
		_setTraceId(this.matrix, traceId);
		traceEnd(3);
	}

	public void get(float[] matrix) {
		_getMatrix(this.matrix, matrix);
	}

	public void invert() {
		traceBegin(4);
		_invert(matrix);
		traceEnd(4);
	}

	public void transpose() {
		traceBegin(5);
		_transpose(matrix);
		traceEnd(5);
	}

	public void postMultiply(Transform transform) {
		traceBegin(6);
		_mul(this.matrix, this.matrix, transform.matrix);
		_setTraceId(matrix, traceId);
		traceEnd(6);
	}

	public void postScale(float sx, float sy, float sz) {
		traceBegin(7);
		_scale(matrix, sx, sy, sz);
		traceEnd(7);
	}

	/**
	 */
	public void postRotate(float angle, float ax, float ay, float az) {
		traceBegin(8);
		_rotate(matrix, angle, ax, ay, az);
		traceEnd(8);
	}

	/**
	 */
	public void postRotateQuat(float qx, float qy, float qz, float qw) {
		traceBegin(9);
		_rotateQuat(matrix, qx, qy, qz, qw);
		traceEnd(9);
	}

	/**
	 */
	public void postTranslate(float tx, float ty, float tz) {
		traceBegin(10);
		_translate(matrix, tx, ty, tz);
		traceEnd(10);
	}

	/**
	 */
	public void transform(float[] v) {
		if ((v.length % 4) != 0) {
			throw new IllegalArgumentException();
		}

		if (v.length != 0) {
			_transformTable(matrix, v);
		}
	}

	/**
	 */
	public void transform(VertexArray in, float[] out, boolean W) {
		if (in == null || out == null) {
			throw new NullPointerException();
		}

		_transformArray(matrix, in.handle, out, W);
	}

	//------------------------------------------------------------------
	// Private methods
	//------------------------------------------------------------------

	// Native methods
	private static native void _mul(byte[] prod, byte[] left, byte[] right);

	private static native void _setIdentity(byte[] matrix);

	private static native void _setTraceId(byte[] matrix, int traceId);

	private static native boolean _traceVerbose();

	private static native void _setMatrix(byte[] matrix, float[] srcMatrix);

	private static native void _getMatrix(byte[] matrix, float[] dstMatrix);

	private static native void _invert(byte[] matrix);

	private static native void _transpose(byte[] matrix);

	private static native void _rotate(byte[] matrix, float angle, float ax, float ay, float az);

	private static native void _rotateQuat(byte[] matrix, float qx, float qy, float qz, float qw);

	private static native void _scale(byte[] matrix, float sx, float sy, float sz);

	private static native void _translate(byte[] matrix, float tx, float ty, float tz);

	private static native void _transformTable(byte[] matrix, float[] v);

	private static native void _transformArray(byte[] matrix, long handle, float[] out, boolean W);

	int traceId() {
		return traceId;
	}

	void traceGet(float[] out) {
		_getMatrix(matrix, out);
	}

	boolean traceIsDegenerate() {
		if (!TRACE_VERBOSE) return false;
		float[] current = new float[16];
		_getMatrix(matrix, current);
		return isDegenerate(current);
	}

	String traceMatrixString() {
		float[] current = new float[16];
		_getMatrix(matrix, current);
		return matrixString(current);
	}

	void traceRender(String point) {
		if (!TRACE_VERBOSE) return;
		float[] current = new float[16];
		_getMatrix(matrix, current);
		if (!isDegenerate(current) || alreadyDumped()) {
			return;
		}
		dumpedIds[dumpCount++] = traceId;
		System.out.println("[M3G TRANSFORM TRACE] degenerate id=" + traceId +
				" point=" + point + " matrix=" + matrixString(current));
		int count = historyCount < HISTORY_SIZE ? historyCount : HISTORY_SIZE;
		int first = historyCount > HISTORY_SIZE ? historyCount - HISTORY_SIZE : 0;
		for (int i = 0; i < count; ++i) {
			int slot = (first + i) % HISTORY_SIZE;
			System.out.println("[M3G TRANSFORM TRACE] id=" + traceId +
					" op=" + opName(historyOps[slot]) +
					" before=" + matrixString(historyBefore, slot * 16) +
					" after=" + matrixString(historyAfter, slot * 16));
		}
	}

	private void traceBegin(int op) {
		if (!TRACE_VERBOSE) return;
		_getMatrix(matrix, traceBefore);
	}

	private void traceEnd(int op) {
		if (!TRACE_VERBOSE) return;
		_getMatrix(matrix, traceAfter);
		if (op == 1 && !isIdentity(traceAfter)) {
			System.out.println("[M3G TRANSFORM TRACE ERROR] id=" + traceId +
					" setIdentity snapshot=" + matrixString(traceAfter));
		}
		int slot = historyCount % HISTORY_SIZE;
		historyOps[slot] = (byte) op;
		System.arraycopy(traceBefore, 0, historyBefore, slot * 16, 16);
		System.arraycopy(traceAfter, 0, historyAfter, slot * 16, 16);
		++historyCount;
	}

	private boolean isDegenerate(float[] m) {
		return Math.abs(m[0]) < 0.000001f && Math.abs(m[1]) < 0.000001f &&
				Math.abs(m[2]) < 0.000001f && Math.abs(m[4]) < 0.000001f &&
				Math.abs(m[5]) < 0.000001f && Math.abs(m[6]) < 0.000001f &&
				Math.abs(m[8]) < 0.000001f && Math.abs(m[9]) < 0.000001f &&
				Math.abs(m[10]) < 0.000001f;
	}

	private boolean isIdentity(float[] m) {
		float[] identity = new float[16];
		identity[0] = identity[5] = identity[10] = identity[15] = 1.0f;
		for (int i = 0; i < 16; ++i) {
			if (Math.abs(m[i] - identity[i]) > 0.000001f) return false;
		}
		return true;
	}

	private boolean alreadyDumped() {
		for (int i = 0; i < dumpCount; ++i) {
			if (dumpedIds[i] == traceId) return true;
		}
		return dumpCount >= MAX_DUMPS;
	}

	private static String matrixString(float[] m) {
		return matrixString(m, 0);
	}

	private static String matrixString(float[] values, int offset) {
		String s = "";
		for (int i = 0; i < 16; ++i) {
			if (i != 0) s += ",";
			s += values[offset + i];
		}
		return s;
	}

	private static String opName(byte op) {
		switch (op) {
			case 1: return "setIdentity";
			case 2: return "setTransform";
			case 3: return "setArray";
			case 4: return "invert";
			case 5: return "transpose";
			case 6: return "mul";
			case 7: return "scale";
			case 8: return "rotate";
			case 9: return "rotateQuat";
			case 10: return "translate";
			default: return "unknown";
		}
	}
}
