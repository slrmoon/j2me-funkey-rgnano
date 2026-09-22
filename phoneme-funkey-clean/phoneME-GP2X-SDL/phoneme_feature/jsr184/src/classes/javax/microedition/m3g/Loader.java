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

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Hashtable;
import java.util.Vector;

import javax.microedition.io.Connector;
import javax.microedition.io.HttpConnection;
import javax.microedition.io.InputConnection;
import javax.microedition.lcdui.Image;
/* import javax.microedition.util.ContextHolder; */

public class Loader {
	// M3G
	static final byte[] M3G_FILE_IDENTIFIER =
			{
					-85, 74, 83, 82, 49, 56, 52, -69, 13, 10, 26, 10
			};
	// PNG
	static final byte[] PNG_FILE_IDENTIFIER =
			{
					-119, 80, 78, 71, 13, 10, 26, 10
			};
	static final int PNG_IHDR = ((73 << 24) + (72 << 16) + (68 << 8) + 82);
	static final int PNG_tRNS = ((116 << 24) + (82 << 16) + (78 << 8) + 83);
	static final int PNG_IDAT = ((73 << 24) + (68 << 16) + (65 << 8) + 84);

	// JPEG
	static final byte[] JPEG_FILE_IDENTIFIER =
			{
					-1, -40
			};
	static final int JPEG_JFIF = ((74 << 24) + (70 << 16) + (73 << 8) + 70);
	// Bytes before colour info in a frame header 'SOFn':
	// length (2 bytes), precision (1 byte), image height & width (4 bytes)
	static final int JPEG_SOFn_DELTA = 7;
	static final int JPEG_INVALID_COLOUR_FORMAT = -1;

	// File identifier types
	private static final int INVALID_HEADER_TYPE = -1;
	private static final int M3G_TYPE = 0;
	private static final int PNG_TYPE = 1;
	private static final int JPEG_TYPE = 2;

	// Misc.
	private static final int MAX_IDENTIFIER_LENGTH = M3G_FILE_IDENTIFIER.length;

	// Initial buffer length for the header
	private static final int AVG_HEADER_SEC_LENGTH = 64;

	// Initial buffer length for the xref section
	private static final int AVG_XREF_SEC_LENGTH = 128;

	// Instance specific
	long handle;

	private Vector iLoadedObjects = new Vector();
	private Vector iFileHistory = new Vector();
	private String iResourceName = null;
	private String iParentResourceName = null;

	private int iTotalFileSize = 0;
	private int iBytesRead = M3G_FILE_IDENTIFIER.length;

	private byte[] iStreamData = null;
	private int iStreamOffset = 0;

	private Interface iInterface;

	//#ifdef RD_JAVA_OMJ
	protected void finalize() {
		doFinalize();
	}
//#endif // RD_JAVA_OMJ

	/**
	 * Default ctor
	 */
	private Loader() {
		iInterface = Interface.getInstance();
	}

	/**
	 * Ctor
	 *
	 * @param aFileHistory        File storage
	 * @param aParentResourceName Resource name
	 */
	private Loader(Vector aFileHistory, String aParentResourceName) {
		iParentResourceName = aParentResourceName;
		iFileHistory = aFileHistory;
		iInterface = Interface.getInstance();
	}

	public static Object3D[] load(String name) throws IOException {
		if (name == null) {
			throw new NullPointerException();
		}

		debug("load(String): " + name);
		try {
			return (new Loader()).loadFromStream(name);
		} catch (SecurityException e) {
			throw e;
		} catch (IOException e) {
			throw e;
		} catch (Exception e) {
			throw new IOException("Load error " + e);
		}
	}

	public static Object3D[] load(byte[] data, int offset) throws IOException {
		if (data == null) {
			throw new NullPointerException();
		}

		if (offset < 0 || offset >= data.length) {
			throw new IndexOutOfBoundsException();
		}
		debug("load(byte[]): offset=" + offset + " length=" + data.length);
		try {
			return (new Loader()).loadFromByteArray(data, offset);
		} catch (SecurityException e) {
			throw e;
		} catch (IOException e) {
			throw e;
		} catch (Exception e) {
			throw new IOException("Load error " + e);
		}
	}

	/**
	 * @see javax.microedition.m3g.Loader#load(String)
	 */
	private Object3D[] loadFromStream(String aName) throws IOException {
		if (aName == null) {
			throw new NullPointerException();
		}

		if (inFileHistory(aName)) {
			throw new IOException("Reference loop detected.");
		}
		iResourceName = aName;
		iFileHistory.addElement(aName);
		PeekInputStream stream = new PeekInputStream(
				getInputStream(aName), MAX_IDENTIFIER_LENGTH);
		// png, jpeg or m3g
		int type = getIdentifierType(stream);
		stream.rewind();
		iStreamData = null;
		iStreamOffset = 0;

		Object3D[] objects;
		try {
			objects = doLoad(stream, type);
		} finally {
			try {
				stream.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		// Finally, remove file from history
		iFileHistory.removeElement(aName);
		return objects;
	}

	/**
	 * @see javax.microedition.m3g.Loader#load(byte[], int)
	 */
	private Object3D[] loadFromByteArray(byte[] aData, int aOffset) throws IOException {
		if (aData == null) {
			throw new NullPointerException("Resource byte array is null.");
		}
		int type = getIdentifierType(aData, aOffset);
		ByteArrayInputStream stream =
				new ByteArrayInputStream(aData, aOffset, aData.length - aOffset);
		iStreamData = aData;
		iStreamOffset = aOffset;
		iResourceName = "ByteArray";

		Object3D[] objects;
		try {
			objects = doLoad(stream, type);
		} finally {
			try {
				stream.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return objects;
	}

	/**
	 * Dispatcher
	 *
	 * @param aStream Source stream
	 * @param aType   Resource type
	 */
	private Object3D[] doLoad(InputStream aStream, int aType) throws IOException {
		debug("doLoad: resource=" + iResourceName + " type=" + typeName(aType));
		switch (aType) {
			case M3G_TYPE:
				return loadM3G(aStream);
			case PNG_TYPE:
				return loadPNG(aStream);
			case JPEG_TYPE:
				return loadJPEG(aStream);
		}
		throw new IOException("File not recognized.");
	}

	/**
	 * PNG resource loader
	 *
	 * @param aStream Resource stream
	 * @return An array of newly created Object3D instances
	 */
	private Object3D[] loadPNG(InputStream aStream) throws IOException {
		int format = Image2D.RGB;
		DataInputStream in = new DataInputStream(aStream);

		// Scan chuncs that have effect on Image2D format
		in.skip(PNG_FILE_IDENTIFIER.length);

		try {
			while (true) {
				int length = in.readInt();
				int type = in.readInt();
				// IHDR
				if (type == PNG_IHDR) {
					in.skip(9);
					int colourType = in.readUnsignedByte();
					length -= 10;

					switch (colourType) {
						case 0:
							format = Image2D.LUMINANCE;
							break;
						case 2:
							format = Image2D.RGB;
							break;
						case 3:
							format = Image2D.RGB;
							break;
						case 4:
							format = Image2D.LUMINANCE_ALPHA;
							break;
						case 6:
							format = Image2D.RGBA;
							break;
					}
				}
				// tRNS
				if (type == PNG_tRNS) {
					switch (format) {
						case Image2D.LUMINANCE:
							format = Image2D.LUMINANCE_ALPHA;
							break;
						case Image2D.RGB:
							format = Image2D.RGBA;
							break;
					}
				}
				// IDAT
				if (type == PNG_IDAT) {
					break;
				}

				in.skip(length + 4);
			}
		}
		// EOF
		catch (Exception e) {
			e.printStackTrace();
		}
		// Close the data stream
		try {
			in.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return buildImage2D(format);
	}

	/**
	 * JPEG (with the same detailed definitions about the JPEG image format as defined in the
	 * JSR 118 MIDP 2.1 specification for LCDUI) MUST be supported by compliant
	 * implementations as a 2D bitmap image format for the Image2D class using the
	 * javax.microedition.m3g.Loader class, and for M3G content files referencing bitmap images.
	 * For colour JPEG images, the pixel format of the returned Image2D object MUST be
	 * Image2D.RGB and for monochrome JPEG images, the pixel format MUST be
	 * Image2D.LUMINANCE.
	 * <p>
	 * JPEG marker: A two-byte code in which the first byte is 0xFF and the second
	 * byte is a value between 1 and 0xFE.
	 * <p>
	 * A JFIF file uses APP0 (0xe0) marker segments and constrains certain parameters in the frame.
	 * <p>
	 * A frame header:
	 * - 0xff, 'SOFn'
	 * - length (2 bytes, Hi-Lo)
	 * - data precision (1 byte)
	 * - image height (2 bytes, Hi-Lo)
	 * - image width (2 bytes, Hi-Lo)
	 * - number of components (1 byte): 1 = grey scaled, 3 = color YCbCr or YIQ, 4 = color CMYK)
	 *
	 * @param aStream Resource stream
	 * @return An array of newly created Object3D instances
	 */
	private Object3D[] loadJPEG(InputStream aStream) throws IOException {
		int format = JPEG_INVALID_COLOUR_FORMAT;
		DataInputStream in = new DataInputStream(aStream);
		// Skip file identifier
		in.skip(JPEG_FILE_IDENTIFIER.length);
		try {
			int marker;
			do {
				// Find marker
				while (in.readUnsignedByte() != 0xff) ;
				do {
					marker = in.readUnsignedByte();
				}
				while (marker == 0xff);

				// Parse marker
				switch (marker) {
					// 'SOFn' (Start Of Frame n)
					case 0xC0:
					case 0xC1:
					case 0xC2:
					case 0xC3:
					case 0xC5:
					case 0xC6:
					case 0xC7:
					case 0xC9:
					case 0xCA:
					case 0xCB:
					case 0xCD:
					case 0xCE:
					case 0xCF:
						// Skip length(2), precicion(1), width(2), height(2)
						in.skip(JPEG_SOFn_DELTA);
						switch (in.readUnsignedByte()) {
							case 1:
								format = Image2D.LUMINANCE;
								break;
							case 3:
								format = Image2D.RGB;
								break;
							default:
								throw new IOException("Unknown JPG format.");
						}
						break;
					// APP0 (0xe0) marker segments and constrains certain parameters in the frame.
					case 0xe0:
						int length = in.readUnsignedShort();
						if (JPEG_JFIF != in.readInt()) {
							throw new IOException("Not a valid JPG file.");
						}
						in.skip(length - 4 - 2);
						break;
					default:
						// Skip variable data
						in.skip(in.readUnsignedShort() - 2);
						break;
				}
			}
			while (format == JPEG_INVALID_COLOUR_FORMAT);
		} catch (Exception e) {
			e.printStackTrace();
		}
		// Close the data stream
		try {
			in.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return buildImage2D(format);
	}

	/**
	 * Image2D builder
	 *
	 * @param aColourFormat Colour format
	 * @return An array of newly created Object3D instances
	 */
	private Object3D[] buildImage2D(int aColourFormat) throws IOException {
		InputStream stream;
		if (iStreamData == null) {
			stream = getInputStream(iResourceName);
		} else {
			stream = new ByteArrayInputStream(iStreamData, iStreamOffset, iStreamData.length - iStreamOffset);
		}
		// Create an image object
		Image2D i2d;
		try {
			i2d = new Image2D(aColourFormat, Image.createImage(stream));
		} finally {
			try {
				stream.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return new Object3D[]{i2d};
	}


	/**
	 * M3G resource loader
	 *
	 * @param aStream Resource stream
	 * @return An array of newly created Object3D instances
	 */
	private Object3D[] loadM3G(InputStream aStream) throws IOException {
		debug("loadM3G: begin resource=" + iResourceName);
		aStream.skip(M3G_FILE_IDENTIFIER.length);
		if (aStream instanceof PeekInputStream)
			((PeekInputStream) aStream).increasePeekBuffer(AVG_HEADER_SEC_LENGTH);

		// Read header
		int compressionScheme = readByte(aStream);
		int totalSectionLength = readUInt32(aStream);
		if (aStream instanceof PeekInputStream && totalSectionLength > AVG_HEADER_SEC_LENGTH)
			((PeekInputStream) aStream).increasePeekBuffer(totalSectionLength - AVG_HEADER_SEC_LENGTH);
		int uncompressedLength = readUInt32(aStream);

		int objectType = readByte(aStream);
		int length = readUInt32(aStream);

		byte vMajor = (byte) readByte(aStream);
		byte vMinor = (byte) readByte(aStream);
		boolean externalLinks = readBoolean(aStream);
		iTotalFileSize = readUInt32(aStream);
		int approximateContentSize = readUInt32(aStream);
		String authoringField = readString(aStream);

		int checksum = readUInt32(aStream);
		debug("loadM3G: header compression=" + compressionScheme +
				" sectionLength=" + totalSectionLength +
				" uncompressedLength=" + uncompressedLength +
				" objectType=" + objectType +
				" objectLength=" + length +
				" version=" + ((int) vMajor) + "." + ((int) vMinor) +
				" externalLinks=" + externalLinks +
				" totalFileSize=" + iTotalFileSize +
				" approxContentSize=" + approximateContentSize +
				" author='" + authoringField + "'" +
				" checksum=" + checksum);

		/* Create and register a new native Loader */
		handle = _ctor(Interface.getHandle());
		Interface.register(this);

		if (externalLinks) {
			if (aStream instanceof PeekInputStream)
				((PeekInputStream) aStream).increasePeekBuffer(AVG_XREF_SEC_LENGTH);
			loadExternalRefs(aStream);
			if (iLoadedObjects.size() > 0)   // Load and set external references
			{
				long[] xRef = new long[iLoadedObjects.size()];
				for (int i = 0; i < xRef.length; i++)
					xRef[i] = ((Object3D) iLoadedObjects.elementAt(i)).handle;
				_setExternalReferences(handle, xRef);
			} else {
				throw new IOException("No external sections [" + iResourceName + "].");
			}
		}

		// Reset stream
		if (aStream instanceof PeekInputStream)
			((PeekInputStream) aStream).rewind();
		else if (aStream.markSupported())
			aStream.reset(); // Reset is supported in ByteArrayInputStreams

		byte[] fileData = new byte[iTotalFileSize];
		int read = 0;
		while (read < iTotalFileSize) {
			int n = aStream.read(fileData, read, iTotalFileSize - read);
			if (n < 0) {
				break;
			}
			read += n;
		}
		int size = _decodeData(handle, 0, fileData);
		debug("loadM3G: decode read=" + read + "/" + iTotalFileSize +
				" next=" + size + " available=" + aStream.available());
		if (size != 0 || read != iTotalFileSize) {
			throw new IOException("Invalid file length [" + iResourceName + "].");
		}

		Object3D[] objects = null;
		int num = _getLoadedObjects(handle, null);
		debug("loadM3G: native object count=" + num);
		if (num > 0) {
			long[] obj = new long[num];
			_getLoadedObjects(handle, obj);
			objects = new Object3D[num];
			for (int i = 0; i < objects.length; i++) {
				objects[i] = Interface.getObjectInstance(obj[i]);
			}
			setUserObjects();
		}
		if (objects == null) {
			debug("loadM3G: native loader returned no objects, parsing M3G sections for " + iResourceName);
			objects = parseM3GObjects(fileData);
		}
		debug("loadM3G: returning objects=" + objects.length + " first=" + objects[0].getClass().getName());
		return objects;
	}

	private Object3D[] parseM3GObjects(byte[] fileData) throws IOException {
		Vector refs = new Vector();
		Vector local = new Vector();
		Hashtable referenced = new Hashtable();
		int pos = M3G_FILE_IDENTIFIER.length;
		for (int i = 0; i < iLoadedObjects.size(); ++i) {
			refs.addElement(iLoadedObjects.elementAt(i));
		}
		while (pos < fileData.length) {
			if (pos + 13 > fileData.length) {
				throw new IOException("Section length mismatch [" + iResourceName + "].");
			}
			int compression = fileData[pos++] & 0xff;
			int totalLength = readUInt32(fileData, pos);
			pos += 4;
			int uncompressedLength = readUInt32(fileData, pos);
			pos += 4;
			int payloadLength = totalLength - 13;
			if (payloadLength < 0 || pos + payloadLength + 4 > fileData.length) {
				throw new IOException("Section length mismatch [" + iResourceName + "].");
			}
			byte[] payload;
			if (compression == 0) {
				if (uncompressedLength != payloadLength) {
					throw new IOException("Section length mismatch [" + iResourceName + "].");
				}
				payload = new byte[payloadLength];
				System.arraycopy(fileData, pos, payload, 0, payloadLength);
			} else if (compression == 1) {
				byte[] compressed = new byte[payloadLength];
				payload = new byte[uncompressedLength];
				System.arraycopy(fileData, pos, compressed, 0, payloadLength);
				if (!_inflate(compressed, payload)) {
					throw new IOException("Decompression error [" + iResourceName + "].");
				}
			} else {
				throw new IOException("Unrecognized compression scheme [" + iResourceName + "].");
			}
			parseM3GSection(payload, refs, local, referenced);
			pos += payloadLength + 4; // checksum
		}
		Vector roots = new Vector();
		for (int i = 0; i < local.size(); ++i) {
			Object3D obj = (Object3D) local.elementAt(i);
			if (obj instanceof World) {
				roots.addElement(obj);
			}
		}
		if (roots.size() == 0) {
			for (int i = 0; i < local.size(); ++i) {
				Object3D obj = (Object3D) local.elementAt(i);
				if (obj instanceof Group && ((Group) obj).getParent() == null) {
					roots.addElement(obj);
				}
			}
		}
		for (int i = 0; i < local.size(); ++i) {
			Object3D obj = (Object3D) local.elementAt(i);
			if (roots.size() == 0 && obj instanceof Node &&
					((Node) obj).getParent() == null) {
				roots.addElement(obj);
			}
		}
		for (int i = 0; i < local.size(); ++i) {
			Object3D obj = (Object3D) local.elementAt(i);
			if (roots.size() == 0 && !referenced.containsKey(obj)) {
				roots.addElement(obj);
			}
		}
		if (roots.size() == 0 && local.size() > 0) {
			roots.addElement(local.elementAt(local.size() - 1));
		}
		if (roots.size() == 0) {
			throw new IOException("No M3G objects [" + iResourceName + "].");
		}
		if (roots.size() > 1 && allRootsAreNodes(roots)) {
			Group root = new Group();
			for (int i = 0; i < roots.size(); ++i) {
				root.addChild((Node) roots.elementAt(i));
			}
			roots.removeAllElements();
			roots.addElement(root);
		}
		Object3D[] out = new Object3D[roots.size()];
		roots.copyInto(out);
		debug("loadM3G: parsed local=" + local.size() + " roots=" + out.length);
		return out;
	}

	private boolean allRootsAreNodes(Vector roots) {
		for (int i = 0; i < roots.size(); ++i) {
			if (!(roots.elementAt(i) instanceof Node)) {
				return false;
			}
		}
		return true;
	}

	private void parseM3GSection(byte[] payload, Vector refs, Vector local,
								 Hashtable referenced) throws IOException {
		Bin in = new Bin(payload);
		while (in.remaining() > 0) {
			int type = in.u8();
			int length = in.u32();
			if (length < 0 || length > in.remaining()) {
				throw new IOException("Object length mismatch [" + iResourceName + "].");
			}
			Bin objIn = in.slice(length);
			Object3D obj = parseM3GObject(type, objIn, refs, referenced);
			objIn.skip(objIn.remaining());
			if (type == 0) {
				continue;
			}
			if (type == 255) {
				if (obj != null) {
					refs.addElement(obj);
				}
				continue;
			}
			if (obj == null) {
				obj = new Group();
			}
			refs.addElement(obj);
			local.addElement(obj);
		}
	}

	private Object3D parseM3GObject(int type, Bin in, Vector refs,
									Hashtable referenced) throws IOException {
		switch (type) {
			case 0:
				return null;
			case 1: {
				AnimationController obj = new AnimationController();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				float speed = in.f32();
				float weight = in.f32();
				int start = in.i32();
				int end = in.i32();
				float position = in.f32();
				int worldTime = in.i32();
				obj.setActiveInterval(start, end);
				obj.setPosition(position, worldTime);
				obj.setSpeed(speed, worldTime);
				obj.setWeight(weight);
				return obj;
			}
			case 2: {
				ObjInfo info = readObject3D(in, refs, referenced);
				Object3D sequence = ref(refs, in.u32(), referenced);
				Object3D controller = ref(refs, in.u32(), referenced);
				AnimationTrack obj = new AnimationTrack((KeyframeSequence) sequence, in.u32());
				applyObject3D(obj, info);
				if (controller instanceof AnimationController) {
					obj.setController((AnimationController) controller);
				}
				return obj;
			}
			case 3: {
				Appearance obj = new Appearance();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				obj.setLayer((byte) in.u8());
				ref(refs, in.u32(), referenced);
				ref(refs, in.u32(), referenced);
				ref(refs, in.u32(), referenced);
				ref(refs, in.u32(), referenced);
				int textures = in.u32();
				for (int i = 0; i < textures; ++i) {
					Object3D texture = ref(refs, in.u32(), referenced);
					if (texture instanceof Texture2D && i < 2) {
						obj.setTexture(i, (Texture2D) texture);
					}
				}
				return obj;
			}
			case 4: {
				Background obj = new Background();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				obj.setColor(in.u32());
				Object3D image = ref(refs, in.u32(), referenced);
				if (image instanceof Image2D) {
					obj.setImage((Image2D) image);
				}
				obj.setImageMode(in.u8(), in.u8());
				obj.setCrop(in.i32(), in.i32(), in.i32(), in.i32());
				obj.setColorClearEnable(in.bool());
				obj.setDepthClearEnable(in.bool());
				return obj;
			}
			case 5: {
				Camera obj = new Camera();
				applyNode(obj, readNode(in, refs, referenced));
				int projection = in.u8();
				if (projection == Camera.PERSPECTIVE) {
					obj.setPerspective(in.f32(), in.f32(), in.f32(), in.f32());
				} else if (projection == Camera.PARALLEL) {
					obj.setParallel(in.f32(), in.f32(), in.f32(), in.f32());
				} else if (projection == Camera.GENERIC) {
					Transform transform = new Transform();
					transform.set(in.f32Array(16));
					obj.setGeneric(transform);
				}
				return obj;
			}
			case 6: {
				CompositingMode obj = new CompositingMode();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				return obj;
			}
			case 7: {
				Fog obj = new Fog();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				return obj;
			}
			case 8: {
				PolygonMode obj = new PolygonMode();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				return obj;
			}
			case 9: {
				Group obj = new Group();
				applyNode(obj, readNode(in, refs, referenced));
				readGroupChildren(obj, in, refs, referenced);
				return obj;
			}
			case 10:
				return readImage2D(in, refs, referenced);
			case 11:
				return readTriangleStripArray(in, refs, referenced);
			case 12: {
				Light obj = new Light();
				applyNode(obj, readNode(in, refs, referenced));
				return obj;
			}
			case 13: {
				Material obj = new Material();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				return obj;
			}
			case 14:
				return readMesh(in, refs, referenced);
			case 15:
				/*
				 * Render the base shape while the software animation path
				 * evaluates morph weights. This preserves the real geometry
				 * and materials instead of replacing MorphingMesh with an
				 * empty node.
				 */
				return readMesh(in, refs, referenced);
			case 17:
				return readTexture2D(in, refs, referenced);
			case 19:
				return readKeyframeSequence(in, refs, referenced);
			case 20:
				return readVertexArray(in, refs, referenced);
			case 21:
				return readVertexBuffer(in, refs, referenced);
			case 22: {
				World obj = new World();
				applyNode(obj, readNode(in, refs, referenced));
				readGroupChildren(obj, in, refs, referenced);
				Object3D camera = ref(refs, in.u32(), referenced);
				Object3D background = ref(refs, in.u32(), referenced);
				if (camera instanceof Camera) {
					obj.setActiveCamera((Camera) camera);
				}
				if (background instanceof Background) {
					obj.setBackground((Background) background);
				}
				return obj;
			}
			case 255: {
				String xref = in.str();
				if (iLoadedObjects.size() > 0) {
					return null;
				}
				return (new Loader(iFileHistory, iResourceName)).loadFromStream(xref)[0];
			}
			default: {
				Group obj = new Group();
				applyObject3D(obj, readObject3D(in, refs, referenced));
				return obj;
			}
		}
	}

	private Image2D readImage2D(Bin in, Vector refs, Hashtable referenced) throws IOException {
		ObjInfo info = readObject3D(in, refs, referenced);
		int format = in.u8();
		boolean mutable = in.bool();
		int width = in.u32();
		int height = in.u32();
		Image2D obj;
		if (mutable) {
			obj = new Image2D(format, width, height);
		} else {
			int paletteLength = in.u32();
			byte[] palette = in.bytes(paletteLength);
			int pixelLength = in.u32();
			byte[] pixels = in.bytes(pixelLength);
			obj = paletteLength > 0 ?
					new Image2D(format, width, height, pixels, palette) :
					new Image2D(format, width, height, pixels);
		}
		applyObject3D(obj, info);
		return obj;
	}

	private KeyframeSequence readKeyframeSequence(Bin in, Vector refs,
												 Hashtable referenced) throws IOException {
		ObjInfo info = readObject3D(in, refs, referenced);
		int interpolation = in.u8();
		int repeat = in.u8();
		int encoding = in.u8();
		int duration = in.u32();
		int first = in.u32();
		int last = in.u32();
		int components = in.u32();
		int count = in.u32();
		float[] bias = null;
		float[] scale = null;
		if (encoding == 1 || encoding == 2) {
			bias = in.f32Array(components);
			scale = in.f32Array(components);
		}
		KeyframeSequence obj = new KeyframeSequence(count, components, interpolation);
		applyObject3D(obj, info);
		for (int i = 0; i < count; ++i) {
			int time = in.u32();
			float[] value = new float[components];
			for (int c = 0; c < components; ++c) {
				if (encoding == 0) {
					value[c] = in.f32();
				} else if (encoding == 1) {
					value[c] = bias[c] + scale[c] * ((float) in.u8() / 255.0f);
				} else {
					value[c] = bias[c] + scale[c] * ((float) in.u16() / 65535.0f);
				}
			}
			obj.setKeyframe(i, time, value);
		}
		obj.setDuration(duration);
		obj.setValidRange(first, last);
		obj.setRepeatMode(repeat);
		return obj;
	}

	private Texture2D readTexture2D(Bin in, Vector refs, Hashtable referenced) throws IOException {
		NodeInfo info = readTransformable(in, refs, referenced);
		Object3D image = ref(refs, in.u32(), referenced);
		Texture2D obj = new Texture2D(image instanceof Image2D ? (Image2D) image : solidImage(0xffffffff));
		applyTransformable(obj, info);
		if (in.remaining() >= 13) {
			obj.setBlendColor(in.u32());
			obj.setBlending(in.u8());
			obj.setWrapping(in.u8(), in.u8());
			obj.setFiltering(in.u8(), in.u8());
		}
		return obj;
	}

	private VertexArray readVertexArray(Bin in, Vector refs, Hashtable referenced) throws IOException {
		ObjInfo info = readObject3D(in, refs, referenced);
		int componentSize = in.u8();
		int components = in.u8();
		int encoding = in.u8();
		int vertices = in.u16();
		VertexArray obj = new VertexArray(vertices, components, componentSize);
		if (componentSize == 1) {
			byte[] values = new byte[vertices * components];
			int[] prev = new int[components];
			for (int i = 0; i < values.length; ++i) {
				int c = i % components;
				int value = in.i8();
				prev[c] = encoding == 0 ? value : prev[c] + value;
				values[i] = (byte) prev[c];
			}
			obj.set(0, vertices, values);
		} else {
			short[] values = new short[vertices * components];
			int[] prev = new int[components];
			for (int i = 0; i < values.length; ++i) {
				int c = i % components;
				int value = in.i16();
				prev[c] = encoding == 0 ? value : prev[c] + value;
				values[i] = (short) prev[c];
			}
			obj.set(0, vertices, values);
		}
		applyObject3D(obj, info);
		return obj;
	}

	private VertexBuffer readVertexBuffer(Bin in, Vector refs, Hashtable referenced) throws IOException {
		VertexBuffer obj = new VertexBuffer();
		applyObject3D(obj, readObject3D(in, refs, referenced));
		obj.setDefaultColor(in.u32());
		Object3D positions = ref(refs, in.u32(), referenced);
		float[] bias = new float[]{in.f32(), in.f32(), in.f32()};
		float scale = in.f32();
		if (positions instanceof VertexArray) {
			obj.setPositions((VertexArray) positions, scale, bias);
		}
		Object3D normals = ref(refs, in.u32(), referenced);
		if (normals instanceof VertexArray) {
			obj.setNormals((VertexArray) normals);
		}
		Object3D colors = ref(refs, in.u32(), referenced);
		if (colors instanceof VertexArray) {
			obj.setColors((VertexArray) colors);
		}
		int texCount = in.u32();
		for (int i = 0; i < texCount; ++i) {
			Object3D tex = ref(refs, in.u32(), referenced);
			bias = new float[]{in.f32(), in.f32(), in.f32()};
			scale = in.f32();
			if (tex instanceof VertexArray && i < 2) {
				obj.setTexCoords(i, (VertexArray) tex, scale, bias);
			}
		}
		return obj;
	}

	private TriangleStripArray readTriangleStripArray(Bin in, Vector refs,
													  Hashtable referenced) throws IOException {
		readObject3D(in, refs, referenced);
		int encoding = in.u8();
		int first = 0;
		int[] indices = null;
		if (encoding == 0) {
			first = in.u32();
		} else if (encoding == 1) {
			first = in.u8();
		} else if (encoding == 2) {
			first = in.u16();
		} else if (encoding == 128 || encoding == 129 || encoding == 130) {
			int count = in.u32();
			indices = new int[count];
			for (int i = 0; i < count; ++i) {
				indices[i] = encoding == 128 ? in.u32() : (encoding == 129 ? in.u8() : in.u16());
			}
		}
		int stripCount = in.u32();
		int[] strips = new int[stripCount];
		for (int i = 0; i < stripCount; ++i) {
			strips[i] = in.u32();
		}
		return indices == null ? new TriangleStripArray(first, strips) :
				new TriangleStripArray(indices, strips);
	}

	private Mesh readMesh(Bin in, Vector refs, Hashtable referenced) throws IOException {
		NodeInfo info = readNode(in, refs, referenced);
		Object3D vb = ref(refs, in.u32(), referenced);
		int count = in.u32();
		IndexBuffer[] indices = new IndexBuffer[count];
		Appearance[] apps = new Appearance[count];
		for (int i = 0; i < count; ++i) {
			Object3D ib = ref(refs, in.u32(), referenced);
			Object3D ap = ref(refs, in.u32(), referenced);
			if (ib instanceof IndexBuffer) {
				indices[i] = (IndexBuffer) ib;
			}
			if (ap instanceof Appearance) {
				apps[i] = (Appearance) ap;
			}
		}
		for (int i = 0; i < indices.length; ++i) {
			if (indices[i] == null) {
				indices[i] = new TriangleStripArray(0, new int[]{3});
			}
		}
		Mesh obj = new Mesh(vb instanceof VertexBuffer ? (VertexBuffer) vb : new VertexBuffer(), indices, apps);
		applyNode(obj, info);
		return obj;
	}

	private void readGroupChildren(Group group, Bin in, Vector refs,
								   Hashtable referenced) throws IOException {
		int childCount = in.u32();
		for (int i = 0; i < childCount; ++i) {
			Object3D child = ref(refs, in.u32(), referenced);
			if (child instanceof Node) {
				group.addChild((Node) child);
			}
		}
	}

	private ObjInfo readObject3D(Bin in, Vector refs, Hashtable referenced) throws IOException {
		ObjInfo info = new ObjInfo();
		info.userID = in.u32();
		int animationTracks = in.u32();
		for (int i = 0; i < animationTracks; ++i) {
			Object3D track = ref(refs, in.u32(), referenced);
			if (track instanceof AnimationTrack) {
				if (info.animationTracks == null) {
					info.animationTracks = new Vector();
				}
				info.animationTracks.addElement(track);
			}
		}
		int params = in.u32();
		if (params > 0) {
			info.userObject = new Hashtable();
			for (int i = 0; i < params; ++i) {
				int id = in.u32();
				int length = in.u32();
				info.userObject.put(new Integer(id), in.bytes(length));
			}
		}
		return info;
	}

	private NodeInfo readTransformable(Bin in, Vector refs, Hashtable referenced) throws IOException {
		NodeInfo info = new NodeInfo();
		ObjInfo obj = readObject3D(in, refs, referenced);
		info.userID = obj.userID;
		info.userObject = obj.userObject;
		info.animationTracks = obj.animationTracks;
		if (in.bool()) {
			info.hasComponentTransform = true;
			info.tx = in.f32();
			info.ty = in.f32();
			info.tz = in.f32();
			info.sx = in.f32();
			info.sy = in.f32();
			info.sz = in.f32();
			info.angle = in.f32();
			info.ax = in.f32();
			info.ay = in.f32();
			info.az = in.f32();
		}
		if (in.bool()) {
			info.matrix = in.f32Array(16);
		}
		return info;
	}

	private NodeInfo readNode(Bin in, Vector refs, Hashtable referenced) throws IOException {
		NodeInfo info = readTransformable(in, refs, referenced);
		info.rendering = in.bool();
		info.picking = in.bool();
		info.alpha = ((float) in.u8()) / 255.0f;
		info.scope = in.u32();
		if (in.bool()) {
			in.skip(2);
			ref(refs, in.u32(), referenced);
			ref(refs, in.u32(), referenced);
		}
		return info;
	}

	private void applyObject3D(Object3D obj, ObjInfo info) {
		obj.setUserID(info.userID);
		if (info.userObject != null) {
			obj.setUserObject(info.userObject);
		}
		if (info.animationTracks != null) {
			for (int i = 0; i < info.animationTracks.size(); ++i) {
				obj.addAnimationTrack((AnimationTrack) info.animationTracks.elementAt(i));
			}
		}
	}

	private void applyTransformable(Transformable obj, NodeInfo info) {
		applyObject3D(obj, info);
		if (info.hasComponentTransform) {
			obj.setTranslation(info.tx, info.ty, info.tz);
			obj.setScale(info.sx, info.sy, info.sz);
			obj.setOrientation(info.angle, info.ax, info.ay, info.az);
		}
		if (info.matrix != null) {
			Transform transform = new Transform();
			transform.set(info.matrix);
			obj.setTransform(transform);
		}
	}

	private void applyNode(Node obj, NodeInfo info) {
		applyTransformable(obj, info);
		obj.setRenderingEnable(info.rendering);
		obj.setPickingEnable(info.picking);
		obj.setAlphaFactor(info.alpha);
		obj.setScope(info.scope);
	}

	private Object3D ref(Vector refs, int encoded, Hashtable referenced) throws IOException {
		if (encoded == 0) {
			return null;
		}
		int index = encoded - 2;
		if (index < 0 || index >= refs.size()) {
			throw new IOException("Reference out of range [" + iResourceName + "].");
		}
		Object3D obj = (Object3D) refs.elementAt(index);
		if (obj != null) {
			referenced.put(obj, obj);
		}
		return obj;
	}

	private static int readUInt32(byte[] data, int offset) {
		return (data[offset] & 0xff)
				| ((data[offset + 1] & 0xff) << 8)
				| ((data[offset + 2] & 0xff) << 16)
				| ((data[offset + 3] & 0xff) << 24);
	}

	private static class ObjInfo {
		int userID;
		Hashtable userObject;
		Vector animationTracks;
	}

	private static class NodeInfo extends ObjInfo {
		boolean hasComponentTransform;
		float tx, ty, tz;
		float sx = 1.0f, sy = 1.0f, sz = 1.0f;
		float angle, ax, ay, az = 1.0f;
		float[] matrix;
		boolean rendering = true;
		boolean picking = true;
		float alpha = 1.0f;
		int scope = -1;
	}

	private static class Bin {
		private byte[] data;
		private int pos;
		private int end;

		Bin(byte[] data) {
			this.data = data;
			this.pos = 0;
			this.end = data.length;
		}

		private Bin(byte[] data, int pos, int end) {
			this.data = data;
			this.pos = pos;
			this.end = end;
		}

		int remaining() {
			return end - pos;
		}

		Bin slice(int length) throws IOException {
			check(length);
			Bin out = new Bin(data, pos, pos + length);
			pos += length;
			return out;
		}

		void check(int length) throws IOException {
			if (length < 0 || pos + length > end) {
				throw new IOException("Unexpected end of M3G object.");
			}
		}

		void skip(int length) throws IOException {
			check(length);
			pos += length;
		}

		int u8() throws IOException {
			check(1);
			return data[pos++] & 0xff;
		}

		int i8() throws IOException {
			check(1);
			return data[pos++];
		}

		boolean bool() throws IOException {
			int b = u8();
			if (b != 0 && b != 1) {
				throw new IOException("Malformed boolean.");
			}
			return b != 0;
		}

		int u16() throws IOException {
			check(2);
			int v = (data[pos] & 0xff) | ((data[pos + 1] & 0xff) << 8);
			pos += 2;
			return v;
		}

		int i16() throws IOException {
			int v = u16();
			return v > 32767 ? v - 65536 : v;
		}

		int u32() throws IOException {
			check(4);
			int v = readUInt32(data, pos);
			pos += 4;
			return v;
		}

		int i32() throws IOException {
			return u32();
		}

		float f32() throws IOException {
			return Float.intBitsToFloat(u32());
		}

		float[] f32Array(int count) throws IOException {
			float[] out = new float[count];
			for (int i = 0; i < count; ++i) {
				out[i] = f32();
			}
			return out;
		}

		byte[] bytes(int length) throws IOException {
			check(length);
			byte[] out = new byte[length];
			System.arraycopy(data, pos, out, 0, length);
			pos += length;
			return out;
		}

		String str() throws IOException {
			ByteArrayInputStream bin = new ByteArrayInputStream(data, pos, remaining());
			String value = readString(bin);
			pos = end - bin.available();
			return value;
		}
	}

	private Image2D solidImage(int argb) {
		byte[] rgba = {
			(byte) ((argb >> 16) & 0xff),
			(byte) ((argb >> 8) & 0xff),
			(byte) (argb & 0xff),
			(byte) ((argb >> 24) & 0xff)
		};
		return new Image2D(Image2D.RGBA, 1, 1, rgba);
	}

	private static void debug(String message) {
		System.out.println("[M3G Loader] " + message);
	}

	private static String typeName(int type) {
		switch (type) {
			case M3G_TYPE:
				return "M3G";
			case PNG_TYPE:
				return "PNG";
			case JPEG_TYPE:
				return "JPEG";
			default:
				return "INVALID";
		}
	}

	/**
	 *
	 */
	private void setUserObjects() throws IOException {
		int numObjects = _getObjectsWithUserParameters(handle, null);
		long[] obj = null;
		if (numObjects > 0) {
			obj = new long[numObjects];
			_getObjectsWithUserParameters(handle, obj);
		}
		for (int i = 0; i < numObjects; i++) {
			int num = _getNumUserParameters(handle, i);
			if (num > 0) {
				Hashtable hash = new Hashtable();
				for (int j = 0; j < num; j++) {
					int len = _getUserParameter(handle, i, j, null);
					byte[] data = new byte[len];
					int id = _getUserParameter(handle, i, j, data);
					/*
					 * Some shipped M3G content contains repeated parameter
					 * identifiers. Keep the final parameter, matching the
					 * section parser and allowing the native scene to load.
					 */
					hash.put(new Integer(id), data);
				}
				Object3D object = Interface.getObjectInstance(obj[i]);
				object.setUserObject(hash);
			}
		}
	}

	/**
	 * Load external resources
	 */
	private void loadExternalRefs(InputStream aStream) throws IOException {
		// Check for the end of the aStream or file
		int firstByte = readByte(aStream);
		if (firstByte == -1 || (iTotalFileSize != 0 && iBytesRead >= iTotalFileSize)) {
			return;
		}

		int compressionScheme = firstByte;

		int totalSectionLength = readUInt32(aStream);
		iBytesRead += totalSectionLength;
		if (aStream instanceof PeekInputStream && totalSectionLength > AVG_XREF_SEC_LENGTH)
			((PeekInputStream) aStream).increasePeekBuffer(totalSectionLength - AVG_XREF_SEC_LENGTH);
		int uncompressedLength = readUInt32(aStream);
		int expectedCount = totalSectionLength;

		// Decompress data if necessary
		CountedInputStream uncompressedStream;
		if (compressionScheme == 0) {
			uncompressedStream = new CountedInputStream(aStream);
			if (uncompressedLength != totalSectionLength - 13) {
				throw new IOException("Section length mismatch [" + iResourceName + "].");
			}
		} else if (compressionScheme == 1) {
			if (uncompressedLength == 0 && totalSectionLength - 13 == 0) {
				uncompressedStream = new CountedInputStream(null);
			} else {
				if (uncompressedLength <= 0 || totalSectionLength - 13 <= 0) {
					throw new IOException("Section length mismatch [" + iResourceName + "].");
				}
				byte[] compressed = new byte[totalSectionLength - 13];
				aStream.read(compressed);

				byte[] uncompressed = new byte[uncompressedLength];

				// zlib decompression
				if (!_inflate(compressed, uncompressed)) {
					throw new IOException("Decompression error.");
				}
				uncompressedStream = new CountedInputStream(
						new ByteArrayInputStream(uncompressed));
			}
		} else {
			throw new IOException("Unrecognized compression scheme [" + iResourceName + "].");
		}

		// load all objects in this section
		uncompressedStream.resetCounter();

		while (uncompressedStream.getCounter() < uncompressedLength) {
			iLoadedObjects.addElement(loadObject(uncompressedStream));
		}

		if (uncompressedStream.getCounter() != uncompressedLength) {
			throw new IOException("Section length mismatch [" + iResourceName + "].");
		}

		// read checksum
		int checksum = readUInt32(aStream);
	}

	private Object3D loadObject(CountedInputStream aStream) throws IOException {
		int objectType = readByte(aStream);
		int length = readUInt32(aStream);

		int expectedCount = aStream.getCounter() + length;
		Object3D newObject;

		if (objectType == 255) {
			String xref = readString(aStream);
			newObject = (new Loader(iFileHistory, iResourceName)).loadFromStream(xref)[0];
		} else {
			throw new IOException("Invalid external section [" + iResourceName + "].");
		}

		if (expectedCount != aStream.getCounter()) {
			throw new IOException("Object length mismatch [" + iResourceName + "].");
		}

		return newObject;
	}

	/**
	 * Read a byte integer from a stream
	 */
	private static final int readByte(InputStream aStream) throws IOException {
		return aStream.read();
	}

	/**
	 * Read a boolean from a stream
	 */
	private static boolean readBoolean(InputStream aStream) throws IOException {
		int b = aStream.read();
		if (b == 0) {
			return false;
		}
		if (b != 1) {
			throw new IOException("Malformed boolean.");
		}
		return true;
	}

	/**
	 * Read a unsigned integer from a stream
	 */
	private static final int readUInt32(InputStream aStream) throws IOException {
		return aStream.read()
				+ (aStream.read() << 8)
				+ (aStream.read() << 16)
				+ (aStream.read() << 24);
	}

	/**
	 * Read a string from a stream
	 */
	private static String readString(InputStream aStream) throws IOException {
		StringBuffer result = new StringBuffer();
		for (int c = aStream.read(); c != 0; c = aStream.read()) {
			if ((c & 0x80) == 0)   // 0xxxxxxx => 1 byte
			{
				result.append((char) (c & 0x00FF));
			} else if ((c & 0xE0) == 0xC0)   // 110xxxxx => 2 bytes
			{
				int c2 = aStream.read();
				if ((c2 & 0xC0) != 0x80)   // second byte is not 10yyyyyy
				{
					throw new IOException("Invalid UTF-8 string.");
				} else   // 110xxxxx 10yyyyyy
				{
					result.append((char) (((c & 0x1F) << 6) | (c2 & 0x3F)));
				}
			} else if ((c & 0xF0) == 0xE0)   // 1110 xxxx => 3 bytes
			{
				int c2 = aStream.read();
				int c3 = aStream.read();
				if (((c2 & 0xC0) != 0x80) || // second byte is not 10yyyyyy
						((c3 & 0xC0) != 0x80))   // third byte is not 10zzzzzz
				{
					throw new IOException("Invalid UTF-8 string.");
				} else   // 1110xxxx 10yyyyyy 10zzzzzz
				{
					result.append((char) (((c & 0x0F) << 12) |
							((c2 & 0x3F) << 6) |
							(c3 & 0x3F)));
				}
			} else   // none of above
			{
				throw new IOException("Invalid UTF-8 string.");
			}
		}

		return result.toString();
	}

	/**
	 * Solve an identifier of the given data
	 *
	 * @param aStream Stream
	 * @return solved identifier.
	 */
	private int getIdentifierType(InputStream aStream) throws IOException {
		byte[] data = new byte[MAX_IDENTIFIER_LENGTH];
		aStream.read(data);
		return getIdentifierType(data, 0);
	}

	/**
	 * Solve an identifier of the given data
	 *
	 * @param aData   Data
	 * @param aOffset Data offset
	 * @return solved identifier.
	 */
	private static int getIdentifierType(byte[] aData, int aOffset) {
		// Try the JPEG/JFIF identifier
		if (parseIdentifier(aData, aOffset, JPEG_FILE_IDENTIFIER)) {
			return JPEG_TYPE;
		}
		// Try the PNG identifier
		else if (parseIdentifier(aData, aOffset, PNG_FILE_IDENTIFIER)) {
			return PNG_TYPE;
		}
		// Try the M3G identifier
		else if (parseIdentifier(aData, aOffset, M3G_FILE_IDENTIFIER)) {
			return M3G_TYPE;
		}
		return INVALID_HEADER_TYPE;
	}

	/**
	 * Parse identifier from a data
	 *
	 * @param aData       Source data
	 * @param aOffset     Source data offset
	 * @param aIdentifier Identifier
	 * @return true if the data contains the given identifier
	 */
	private static boolean parseIdentifier(byte[] aData, int aOffset, byte[] aIdentifier) {
		if ((aData.length - aOffset) < aIdentifier.length) {
			return false;
		}
		for (int index = 0; index < aIdentifier.length; index++) {
			if (aData[index + aOffset] != aIdentifier[index]) {
				return false;
			}
		}
		return true;
	}

	/**
	 * File name storage for preventing multiple referencing
	 *
	 * @param name File name
	 * @return true if the storage contains the given file name
	 */
	private boolean inFileHistory(String name) {
		for (int i = 0; i < iFileHistory.size(); i++)
			if ((iFileHistory.elementAt(i)).equals(name)) {
				return true;
			}
		return false;
	}

	/*
	 * InputStream-related helper functions
	 */

	/**
	 * Open a HTTP stream and check its MIME type
	 *
	 * @param name Resource name
	 * @return a http stream and checks the MIME type
	 */
	private InputStream getHttpInputStream(String name) throws IOException {
		InputConnection ic = (InputConnection) Connector.open(name);
		// Content-Type is available for http and https connections
		if (ic instanceof HttpConnection) {
			HttpConnection hc = (HttpConnection) ic;
			// Check MIME type
			String type = hc.getHeaderField("Content-Type");
			if (type != null &&
					!type.equals("application/m3g") &&
					!type.equals("image/png") &&
					!type.equals("image/jpeg")) {
				throw new IOException("Wrong MIME type: " + type + ".");
			}
		}

		InputStream is;
		try {
			is = ic.openInputStream();
		} finally {
			try {
				ic.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return is;
	}

	// returns a stream built from the specified file or URI
	private InputStream getInputStream(String name) throws IOException {
		if (name.indexOf(':') != -1)   // absolute URI reference
		{
			return getHttpInputStream(name);
		}

		if (name.charAt(0) == '/' || iParentResourceName == null)   // absolute file reference
		{
			return getClass().getResourceAsStream(name);
		}

		String uri = iParentResourceName.substring(0, iParentResourceName.lastIndexOf('/') + 1) + name;

		if (uri.charAt(0) == '/') {
			return getClass().getResourceAsStream(uri);
		} else {
			return getHttpInputStream(uri);
		}
	}

	class PeekInputStream extends InputStream {
		private int[] iPeekBuffer;
		private InputStream iStream;
		private int iBuffered;
		private int iCounter;

		PeekInputStream(InputStream aStream, int aLength) {
			iStream = aStream;
			iPeekBuffer = new int[aLength];
		}

		public int read() throws IOException {
			if (iCounter < iBuffered) {
				return iPeekBuffer[iCounter++];
			}

			int nv = iStream.read();

			if (iBuffered < iPeekBuffer.length) {
				iPeekBuffer[iBuffered] = nv;
				iBuffered++;
			}

			iCounter++;
			return nv;
		}

		public void increasePeekBuffer(int aLength) {
			int[] temp = new int[iPeekBuffer.length + aLength];
			System.arraycopy(iPeekBuffer, 0, temp, 0, iBuffered);
			iPeekBuffer = temp;
		}

		public int available() throws IOException {
			if (iCounter < iBuffered) {
				return iBuffered - iCounter + iStream.available();
			}
			return iStream.available();
		}

		public void close() {
			try {
				iStream.close();
			} catch (IOException ioe) {
				// Intentionally left empty
			}
		}

		public void rewind() throws IOException {
			if (iCounter > iBuffered) {
				throw new IOException("Peek buffer overrun.");
			}
			iCounter = 0;
		}
	}

	class CountedInputStream extends InputStream {
		private InputStream iStream;
		private int iCounter;

		public CountedInputStream(InputStream aStream) {
			iStream = aStream;
			resetCounter();
		}

		public int read() throws IOException {
			iCounter++;
			return iStream.read();
		}

		public void resetCounter() {
			iCounter = 0;
		}

		public int getCounter() {
			return iCounter;
		}

		public void close() {
			try {
				iStream.close();
			} catch (IOException ioe) {
				// Intentionally left empty
			}
		}

		public int available() throws IOException {
			return iStream.available();
		}
	}

	//#ifdef RD_JAVA_OMJ
	private void doFinalize() {
		registeredFinalize();
	}
//#endif // RD_JAVA_OMJ

	// Finalization method for Symbian
	private void registeredFinalize() {
		if (handle != 0) {
			Platform.finalizeObject(handle, iInterface);
			Interface.deregister(this, iInterface);
			iInterface = null;
			handle = 0;
		}
	}

	// zlib decompression
	private native static boolean _inflate(byte[] data, byte[] buffer);

	// native loader
	private native static long _ctor(long handle);

	private native static int _decodeData(long handle, int offset, byte[] data);

	private native static void _setExternalReferences(long handle, long[] references);

	private native static int _getLoadedObjects(long handle, long[] objects);

	private native static int _getObjectsWithUserParameters(long handle, long[] objects);

	private native static int _getNumUserParameters(long handle, int obj);

	private native static int _getUserParameter(long handle, int obj, int index, byte[] data);
}
