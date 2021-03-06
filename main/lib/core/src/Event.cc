#include <ostream>
#include <iostream>
#include <time.h>

#include "Event.hh"
#include "PluginManager.hh"
#include "BufferSerializer.hh"

namespace eudaq {

  template class DLLEXPORT Factory<Event>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(Deserializer&)>&
  Factory<Event>::Instance<Deserializer&>();
    
  namespace {
    static const char * const FLAGNAMES[] = {
      "BORE",
      "EORE",
      "HITS",
      "FAKE",
      "SIMU",
      "EUDAQ2",
      "Packet"
    };
  }

  Event::Event(Deserializer & ds) {
    ds.read(m_typeid);
    ds.read(m_flags);
    ds.read(m_runnumber);
    ds.read(m_eventnumber);
    if (IsEUDAQ2()){
      ds.read(m_timestamp);
    }
    else{
      uint64_t timestamp;
      ds.read(timestamp);
      m_timestamp.push_back(timestamp);
    }
    ds.read(m_tags);
  }

  Event::Event():m_flags(0), m_runnumber(0), m_eventnumber(0){
    
  }

  
  Event::Event(unsigned run, unsigned event, timeStamp_t timestamp /*= NOTIMESTAMP*/, unsigned flags /*= 0*/) : m_flags(flags | FLAG_EUDAQ2), // it is not desired that user use old EUDAQ 1 event format. If one wants to use it one has clear the flags first and then set flags with again.
    m_runnumber(run),
    m_eventnumber(event)
  {
    m_timestamp.push_back(timestamp);
  }

  void Event::Serialize(Serializer & ser) const {
    ser.write(m_typeid);
#ifdef __FORCE_EUDAQ1_FILES___
    auto dummy = m_flags & ~Event::FLAG_EUDAQ2;
    ser.write(m_flags & ~Event::FLAG_EUDAQ2);
#else
    ser.write(m_flags);
#endif
    
    ser.write(m_runnumber);
    ser.write(m_eventnumber);
#ifdef __FORCE_EUDAQ1_FILES___

    ser.write(m_timestamp.at(0));
#else

    if (IsEUDAQ2())
    {
      ser.write(m_timestamp);
      
    }
    else
    {
      ser.write(m_timestamp.at(0));
    }
#endif
    ser.write(m_tags);
  }

  void Event::Print(std::ostream & os) const {
    Print(os, 0);
  }

  void Event::Print(std::ostream & os, size_t offset) const
  {
    os << std::string(offset, ' ') << "<Event> \n";
    os << std::string(offset + 2, ' ') << "<Type>" << id2str(get_id()) << ":" << GetSubType() << "</Type> \n";
    os << std::string(offset + 2, ' ') << "<RunNumber>" << m_runnumber << "</RunNumber>\n";
    os << std::string(offset + 2, ' ') << "<EventNumber>" << m_eventnumber << "</EventNumber>\n";
    
    bool timestampsSet = false;
    for (auto& e : m_timestamp)
    {
      if (e!=NOTIMESTAMP)
      {
        if (!timestampsSet)
        {
          os << std::string(offset + 2, ' ') << "<Time> \n";
          timestampsSet = true;
        }
        os << std::string(offset +4, ' ') << "0x" << to_hex(e, 16) << ",\n";

      }
    }
    if (timestampsSet)
    {
      os << std::string(offset + 2, ' ') << "</Time>\n";
    }
    if (m_timestamp[0] != NOTIMESTAMP)
    if (m_flags) {
      unsigned f = m_flags;
      bool first = true;
      for (size_t i = 0; f > 0; ++i, f >>= 1) {
        if (f & 1) {
          if (first)
          {
            os << std::string(offset + 2, ' ') << "<Flags> ";
              first = false;
          }
          else
          {
            os << ", ";
          }
          if (i < sizeof FLAGNAMES / sizeof *FLAGNAMES)
          {
            os << std::string(FLAGNAMES[i]);
          }
          else{
            os << to_string(i);
          }
        }
      }
      if (first==false)
      {
        os  << "</Flags> \n";
      }

    }
    if (m_tags.size() > 0) {
      os << std::string(offset + 2, ' ') << "<Tags> \n";
      for (map_t::const_iterator i = m_tags.begin(); i != m_tags.end(); ++i) {
        os << std::string(offset+4, ' ')  << i->first << "=" << i->second << "\n";
      }
      os << std::string(offset + 2, ' ') << "</Tags> \n";
    }
    os << std::string(offset, ' ') << "</Event> \n";
  }

  unsigned Event::str2id(const std::string & str) {
    uint32_t result = 0;
    for (size_t i = 0; i < 4; ++i) {
      if (i < str.length()) result |= str[i] << (8 * i);
    }
    return result;
  }

  std::string Event::id2str(unsigned id) {
    std::string result(4, '\0');
    for (int i = 0; i < 4; ++i) {
      result[i] = (char)(id & 0xff);
      id >>= 8;
    }
    for (int i = 3; i >= 0; --i) {
      if (result[i] == '\0') {
        result.erase(i);
        break;
      }
    }
    return result;
  }

  EventUP Event::Create(Deserializer &ds){
    uint32_t id;
    ds.PreRead(id);
    return Factory<Event>::Create<Deserializer&>(id, ds);
  }
  
  Event & Event::SetTag(const std::string & name, const std::string & val) {
    m_tags[name] = val;
    return *this;
  }

  std::string Event::GetTag(const std::string & name, const std::string & def) const {
    auto i = m_tags.find(name);
    if (i == m_tags.end()) return def;
    return i->second;
  }

  void Event::SetTimeStampToNow(size_t i)
  {
    m_timestamp[i]=static_cast<uint64_t>(clock());
  }

  bool Event::HasTag(const std::string &name) const{
    if (m_tags.find(name) == m_tags.end())
      return false;
    else
      return true;
  }

  void Event::pushTimeStampToNow()
  {
    m_timestamp.push_back(static_cast<uint64_t>(clock()));
  }

  unsigned Event::GetRunNumber() const
  {
    return m_runnumber;
  }

  unsigned Event::GetEventNumber() const
  {
    return m_eventnumber;
  }

  Event::timeStamp_t Event::GetTimestamp(size_t i/*=0*/) const
  {
    return m_timestamp[i];
  }

  size_t Event::GetSizeOfTimeStamps() const
  {
    return m_timestamp.size();
  }

  void Event::setTimeStamp(timeStamp_t timeStamp, size_t i/*=0*/)
  {
    if (i>=m_timestamp.size())
    {
      size_t oldSize = m_timestamp.size();
      m_timestamp.resize(i + 1);
      for (auto j = oldSize; j < m_timestamp.size();++j)
      {
        m_timestamp[j] = NOTIMESTAMP;
      }

    }
    m_timestamp[i] = timeStamp;
  }



  uint64_t Event::getUniqueID() const
  {
    uint64_t id = PluginManager::getUniqueIdentifier(*this);
    id = id << 32;
    id |= GetEventNumber();
    return id;
  }

  Event* Event::Clone() const{ //TODO: clone directly
    BufferSerializer ser;
    Serialize(ser);
    uint32_t id;
    ser.PreRead(id);
    Event* ev = Factory<Event>::Create<Deserializer&>(id, ser).release();
    return ev;
  }
  
  std::ostream & operator << (std::ostream &os, const Event &ev) {
    ev.Print(os);
    return os;
  }

}
