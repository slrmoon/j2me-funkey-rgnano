package javax.microedition.location;

public class Landmark {
    private String name;
    private String description;
    private QualifiedCoordinates coordinates;
    private AddressInfo addressInfo;

    public Landmark(String name, String description, QualifiedCoordinates coordinates,
            AddressInfo addressInfo) {
        setName(name);
        this.description = description;
        this.coordinates = coordinates;
        this.addressInfo = addressInfo;
    }

    public String getName() {
        return name;
    }

    public String getDescription() {
        return description;
    }

    public QualifiedCoordinates getQualifiedCoordinates() {
        return coordinates;
    }

    public AddressInfo getAddressInfo() {
        return addressInfo;
    }

    public void setQualifiedCoordinates(QualifiedCoordinates coordinates) {
        this.coordinates = coordinates;
    }

    public void setName(String name) {
        if (name == null) {
            throw new NullPointerException();
        }
        this.name = name;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public void setAddressInfo(AddressInfo addressInfo) {
        this.addressInfo = addressInfo;
    }
}
