#include "connectionkind.h"

namespace oi{

ConnectionKind::ConnectionKind(const QString &id, const QString &displayName)
    : m_id(id), m_displayName(displayName){
}

const ConnectionKind &ConnectionKind::network(){
    static const ConnectionKind kind("network", "Network");
    return kind;
}

const ConnectionKind &ConnectionKind::serial(){
    static const ConnectionKind kind("serial", "Serial");
    return kind;
}

QList<ConnectionKind> ConnectionKind::knownKinds(){
    return { ConnectionKind::network(), ConnectionKind::serial() };
}

ConnectionKind ConnectionKind::byId(const QString &id){
    for(const ConnectionKind &kind : ConnectionKind::knownKinds()){
        if(kind.id() == id){
            return kind;
        }
    }
    return ConnectionKind();
}

const QString &ConnectionKind::id() const{
    return this->m_id;
}

const QString &ConnectionKind::displayName() const{
    return this->m_displayName;
}

bool ConnectionKind::isValid() const{
    return !this->m_id.isEmpty();
}

bool ConnectionKind::operator==(const ConnectionKind &other) const{
    return this->m_id == other.m_id;
}

bool ConnectionKind::operator!=(const ConnectionKind &other) const{
    return !(*this == other);
}

}
