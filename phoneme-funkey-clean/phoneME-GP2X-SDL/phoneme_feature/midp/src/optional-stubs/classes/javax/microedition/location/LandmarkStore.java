package javax.microedition.location;

import java.io.IOException;
import java.util.Enumeration;

/** FunKey: no landmark database; there are no stores and none can be made. */
public class LandmarkStore {
    private LandmarkStore() {
    }

    public static synchronized LandmarkStore getInstance(String storeName) {
        return null;
    }

    public static void createLandmarkStore(String storeName) throws IOException, LandmarkException {
        if (storeName == null) {
            throw new NullPointerException();
        }
        throw new LandmarkException("landmark stores not available");
    }

    public static void deleteLandmarkStore(String storeName) throws IOException, LandmarkException {
        if (storeName == null) {
            throw new NullPointerException();
        }
    }

    public static String[] listLandmarkStores() throws IOException {
        return null;
    }

    public void addLandmark(Landmark landmark, String category) throws IOException {
        throw new IOException("landmark stores not available");
    }

    public Enumeration getLandmarks(String category, String name) throws IOException {
        return null;
    }

    public Enumeration getLandmarks() throws IOException {
        return null;
    }

    public Enumeration getLandmarks(String category, double minLatitude, double maxLatitude,
            double minLongitude, double maxLongitude) throws IOException {
        return null;
    }

    public void removeLandmarkFromCategory(Landmark lm, String category) throws IOException {
    }

    public void updateLandmark(Landmark landmark) throws IOException, LandmarkException {
        throw new LandmarkException("landmark stores not available");
    }

    public void deleteLandmark(Landmark lm) throws IOException, LandmarkException {
        throw new LandmarkException("landmark stores not available");
    }

    public Enumeration getCategories() {
        return null;
    }

    public void addCategory(String categoryName) throws LandmarkException, IOException {
        throw new LandmarkException("landmark stores not available");
    }

    public void deleteCategory(String categoryName) throws LandmarkException, IOException {
        throw new LandmarkException("landmark stores not available");
    }
}
