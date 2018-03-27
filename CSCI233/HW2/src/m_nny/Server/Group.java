package m_nny.Server;

import java.util.LinkedList;
import java.util.List;

public class Group {
    private List<Session> members;
    private String name;
    Group(String name) {
        this.name = name;
        members = new LinkedList<>();
    }

    void addMember(Session client) {
        members.add(client);
    }

    void removeMember(Session client) {
        members.remove(client);
    }
    @Override
    public boolean equals(Object aThat) {
        if (this == aThat)
            return true;
        if (!(aThat instanceof Group))
            return false;
        Group that = (Group)aThat;
        return this.name.equals(that.name);
    }

    @Override
    public String toString() {
        StringBuilder result = new StringBuilder(name + ": ");
        for (Session member: members) {
            result.append(member.getUsername()).append(", ");
        }
        if (!members.isEmpty())
            result.setLength(result.length() - 2);
        return result.toString();
    }

    public List<Session> getMembers() {
        return members;
    }

    public String getName() {
        return name;
    }
}
