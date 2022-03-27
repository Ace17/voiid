#include "base/error.h"
#include "base/matrix4.h"
#include "base/mesh.h"
#include "base/span.h"
#include "base/util.h" // baseName
#include "misc/decompress.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring> // memcmp
#include <map>

namespace
{
static const Matrix4f identity = scale({ 1, 1, 1 });

///////////////////////////////////////////////////////////////////////////////

constexpr uint32_t hashed(String str)
{
  // 32-bit FNV-1a
  uint32_t val = 2166136261u;

  for(auto c : str)
    val = (c ^ val) * 16777619u;

  return val;
}

///////////////////////////////////////////////////////////////////////////////
// Low-level parsing: tokenization

int parseUint8(Span<const uint8_t>& stream)
{
  int r = stream[0];
  stream += 1;
  return r;
}

uint32_t parseUint32(Span<const uint8_t>& stream)
{
  uint32_t r = 0;

  for(int k = 0; k < 4; ++k)
  {
    r |= (stream[0] << (8 * k));
    stream += 1;
  }

  return r;
}

int64_t parseSint64(Span<const uint8_t>& stream)
{
  int64_t r = 0;

  for(int k = 0; k < 8; ++k)
  {
    r |= (int64_t(stream[0]) << (8 * k));
    stream += 1;
  }

  return r;
}

double parseFloat(Span<const uint8_t>& stream)
{
  float r;
  memcpy(&r, stream.data, 4);
  stream += 4;
  return r;
}

double parseDouble(Span<const uint8_t>& stream)
{
  double r;
  memcpy(&r, stream.data, 8);
  stream += 8;
  return r;
}

enum class Token
{
  ObjectBegin,
  Value,
  ObjectEnd,
  EndOfFile,
};

enum class ValueType
{
  Unknown,
  Integer,
  Double,
  String,
  RawArray,
  FloatArray,
  DoubleArray,
  IntegerArray,
  LongArray,
  BoolArray,
};

void enforce(bool condition, const char* fmt, ...)
{
  if(condition)
    return;

  char buffer[256];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
  va_end(args);

  throw Error(String(buffer, strlen(buffer)));
}

// FBX tokenizer: produces a linear sequence of tokens.
// These tokens are accessed one by one, through a 'read' API (pull)
struct Tokenizer
{
  Tokenizer(Span<const uint8_t> input, const uint8_t* base_) :  m_base(base_)
  {
    m_streamStack.push_back(input);
    nextToken();
  }

  void nextToken()
  {
    if(!subReadToken())
      tokenType = Token::EndOfFile;
  }

  bool subReadToken()
  {
    if(m_streamStack.empty())
      return false;

    auto& stream = m_streamStack.back();

    assert(stream.len >= 0);
    switch(m_state)
    {
    case State::ObjectStart:
      {
        static const uint8_t terminator[13] = {};

        if(stream.len >= 13 && memcmp(stream.data, terminator, 13) == 0)
          return false;

        const uint32_t endOffset = parseUint32(stream);
        const uint32_t valueCount = parseUint32(stream);
        const uint32_t valueSize = parseUint32(stream);
        (void)valueSize;

        const auto end = m_base + endOffset;
        const auto objSize = int(end - stream.data);

        auto objData = stream.sub(objSize);
        stream += objSize;

        const int nameLen = parseUint8(objData);

        tokenType = Token::ObjectBegin;
        m_valueCount = valueCount;

        resizeScratchBuffer(nameLen + 1);
        auto dataBuffer = (uint8_t*)m_scratchBuffer.data();

        for(int k = 0; k < nameLen; ++k)
          dataBuffer[k] = parseUint8(objData);

        dataBuffer[nameLen] = 0;

        objName.data = (const char*)dataBuffer;
        objName.len = nameLen;

        m_streamStack.push_back(objData);

        if(m_valueCount > 0)
        {
          m_state = State::Values;
        }
        else if(objData.len > 13)
        {
          m_state = State::ObjectStart; // child
        }
        else
        {
          m_state = State::ObjectEnd;
        }
      }
      break;
    case State::ObjectEnd:
      {
        assert(stream.len <= 13);
        tokenType = Token::ObjectEnd;
        m_streamStack.pop_back();

        // next sibling ?
        if(!m_streamStack.empty() && m_streamStack.back().len > 13)
          m_state = State::ObjectStart;
      }
      break;
    case State::Values:
      {
        assert(m_valueCount > 0);
        parseValue(stream);

        --m_valueCount;

        tokenType = Token::Value;

        if(m_valueCount == 0)
        {
          if(0)
          {
            if(stream.len > 0)
            {
              printf("(WARNING: end of values, %d bytes left in substream)\n", stream.len);

              for(int i = 0; i < stream.len; ++i)
              {
                printf("%.2X ", stream[i]);

                if((i + 1) % 32 == 0)
                  printf("\n");
              }

              printf("\n");
            }
          }

          if(stream.len <= 13)
            m_state = State::ObjectEnd; // end of outer object
          else
            m_state = State::ObjectStart; // beginning of sub object
        }
      }
      break;
    }

    return true;
  }

  void parseValue(Span<const uint8_t>& s)
  {
    enforce(s.len > 0, "FBX: unexpected end of values");

    valueType = ValueType::Unknown;

    const uint8_t propCode = parseUint8(s);
    switch(propCode)
    {
    case 'S': // string
      valueType = ValueType::String;
      m_valueString.len = parseUint32(s);
      m_valueString.data = (const char*)s.data;
      s += m_valueString.len;
      break;
    case 'R': // raw
      valueType = ValueType::RawArray;
      m_valueRawArray.len = parseUint32(s);
      m_valueRawArray.data = s.data;
      s += m_valueRawArray.len;
      break;
    case 'C': // 1 byte boolean
      valueType = ValueType::Integer;
      m_valueInt = s[0] != 0 ? 1 : 0;
      s += 1;
      break;
    case 'F': // 4 byte float
      valueType = ValueType::Double;
      m_valueDouble = parseFloat(s);
      break;
    case 'D': // 8 byte double-precision IEEE 754 number
      valueType = ValueType::Double;
      m_valueDouble = parseDouble(s);
      break;
    case 'I': // 4 byte signed integer
      valueType = ValueType::Integer;
      m_valueInt = (int32_t)parseUint32(s);
      break;
    case 'L': // 8 byte signed integer
      valueType = ValueType::Integer;
      m_valueInt = parseSint64(s);
      break;
    case 'b': // array of 1-byte boolean
      valueType = ValueType::BoolArray;
      m_valueBoolArray.len = parseArray(s, 1);
      m_valueBoolArray.data = (uint8_t*)m_scratchBuffer.data();
      break;
    case 'i': // array of 'I'
      valueType = ValueType::IntegerArray;
      m_valueIntegerArray.len = parseArray(s, 4);
      m_valueIntegerArray.data = (int*)m_scratchBuffer.data();
      break;
    case 'f': // array of 'F'
      valueType = ValueType::FloatArray;
      m_valueFloatArray.len = parseArray(s, 4);
      m_valueFloatArray.data = (float*)m_scratchBuffer.data();
      break;
    case 'l': // array of 8-byte integers
      valueType = ValueType::LongArray;
      m_valueLongArray.len = parseArray(s, 8);
      m_valueLongArray.data = (int64_t*)m_scratchBuffer.data();
      break;
    case 'd': // array of 'D'
      valueType = ValueType::DoubleArray;
      m_valueDoubleArray.len = parseArray(s, 8);
      m_valueDoubleArray.data = (double*)m_scratchBuffer.data();
      break;
    default:
      char buffer[128];
      throw Error(format(buffer, "Unsupported FBX file (prop code '%c')", propCode));
    }
  }

  // the resulting array is in the scratch buffer
  int parseArray(Span<const uint8_t>& s, int elementSize)
  {
    const uint32_t arrayLength = parseUint32(s);
    const uint32_t encoding = parseUint32(s);
    const uint32_t compressedLength = parseUint32(s);

    resizeScratchBuffer(arrayLength * elementSize);

    if(encoding == 0)
    {
      memcpy(m_scratchBuffer.data(), s.data, arrayLength * elementSize);
      s += arrayLength * elementSize;
    }
    else
    {
      auto decompressedData = zlibDecompress(s.sub(compressedLength));
      memcpy(m_scratchBuffer.data(), decompressedData.data(), arrayLength * elementSize);
      s += compressedLength;
    }

    return arrayLength;
  }

  enum class State
  {
    ObjectStart, // About to read an object header
    Values, // About to read a value (aka 'Property')
    ObjectEnd, // About to (virtually) read an object end
  };

  // input FSM
  const uint8_t* const m_base;
  State m_state = State::ObjectStart;
  std::vector<Span<const uint8_t>> m_streamStack;
  int m_valueCount {};

  // public info about current token
  Token tokenType {};
  String objName {};
  ValueType valueType {};

  union
  {
    String m_valueString;
    Span<const uint8_t> m_valueRawArray;
    Span<const float> m_valueFloatArray;
    Span<const double> m_valueDoubleArray;
    Span<const uint8_t> m_valueBoolArray;
    Span<const int> m_valueIntegerArray;
    Span<const int64_t> m_valueLongArray;
    int64_t m_valueInt;
    double m_valueDouble;
  };

  // internal storage
  std::vector<uint64_t> m_scratchBuffer;

  void resizeScratchBuffer(size_t n)
  {
    auto CeilDiv = [] (int num, int divisor) { return (num + divisor + 1) / divisor; };
    m_scratchBuffer.resize(CeilDiv(n, sizeof(m_scratchBuffer[0])));
  }
};

////////////////////////////////////////////////////////////////////////////////
// High-level parsing

void skipValues(Tokenizer& tokenizer)
{
  while(tokenizer.tokenType == Token::Value)
    tokenizer.nextToken();
}

void expect(Tokenizer& tokenizer, Token type)
{
  enforce(tokenizer.tokenType == type, "Expected token type object %d, got %d", (int)type, (int)tokenizer.tokenType);
  tokenizer.nextToken();
}

String expectObjectBegin(Tokenizer& tokenizer)
{
  enforce(tokenizer.tokenType == Token::ObjectBegin, "Expected token type 'object begin', got %d", (int)tokenizer.tokenType);
  return tokenizer.objName;
}

uint32_t expectInteger(Tokenizer& tokenizer)
{
  if(tokenizer.tokenType != Token::Value)
  {
    char buffer[256];
    throw Error(format(buffer, "Expected token type object value, got %d", (int)tokenizer.tokenType));
  }

  if(tokenizer.valueType != ValueType::Integer)
  {
    char buffer[256];
    throw Error(format(buffer, "Expected token value integer, got %d", (int)tokenizer.valueType));
  }

  const uint32_t val = tokenizer.m_valueInt;
  tokenizer.nextToken();

  return val;
}

double expectDecimalNumber(Tokenizer& tokenizer)
{
  if(tokenizer.tokenType != Token::Value)
  {
    char buffer[256];
    throw Error(format(buffer, "Expected token type object value, got %d", (int)tokenizer.tokenType));
  }

  double val = 0;

  if(tokenizer.valueType == ValueType::Double)
    val = tokenizer.m_valueDouble;
  else if(tokenizer.valueType == ValueType::Integer)
    val = tokenizer.m_valueInt;
  else
  {
    char buffer[256];
    throw Error(format(buffer, "Expected token value double, got %d", (int)tokenizer.valueType));
  }

  tokenizer.nextToken();

  return val;
}

String expectString(Tokenizer& tokenizer)
{
  if(tokenizer.tokenType != Token::Value)
  {
    char buffer[256];
    throw Error(format(buffer, "Expected token type object value, got %d", (int)tokenizer.tokenType));
  }

  if(tokenizer.valueType != ValueType::String)
  {
    char buffer[256];
    throw Error(format(buffer, "Expected token value string, got %d", (int)tokenizer.valueType));
  }

  const auto val = tokenizer.m_valueString;
  tokenizer.nextToken();

  return val;
}

void skipObject(Tokenizer& tokenizer)
{
  expect(tokenizer, Token::ObjectBegin);

  skipValues(tokenizer);

  // skip children
  while(tokenizer.tokenType != Token::ObjectEnd)
    skipObject(tokenizer);

  expect(tokenizer, Token::ObjectEnd);
}

struct FbxParser
{
public:
  ImportedScene parse(Tokenizer& tokenizer)
  {
    while(tokenizer.tokenType != Token::EndOfFile)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      case hashed("Objects"):
        parseSection_Objects(tokenizer);
        break;
      case hashed("Connections"):
        parseSection_Connections(tokenizer);
        break;
      default:
        skipObject(tokenizer);
        break;
      }
    }

    processConnections();

    return finish();
  }

private:
  // "Connections" pass
  void processConnections()
  {
    for(auto& conn : m_connections)
      for(auto& child : conn.second)
        connect(conn.first, child);
  }

  void connect(uint32_t parent, uint32_t child)
  {
    auto entry = table.find(parent);
    switch(entry->second.type)
    {
    case HandleEntry::Type::Model:
      connectModel(models[entry->second.index], child);
      break;
    case HandleEntry::Type::Material:
      connectMaterial(materials[entry->second.index], child);
      break;
    default:
      break;
    }
  }

  struct FbxModel;
  struct FbxMaterial;
  struct FbxGeometry;
  struct FbxLight;

  void connectModel(FbxModel& model, uint32_t child)
  {
    auto entry = table.find(child);
    enforce(entry != table.end(), "Unexisting child handle for model '%.*s' : '%.8X'", model.name.len, model.name.data, child);
    switch(entry->second.type)
    {
    case HandleEntry::Type::Geometry:
      model.geometry.push_back(&geometry[entry->second.index]);
      break;
    case HandleEntry::Type::Material:
      model.materials.push_back(&materials[entry->second.index]);
      break;
    case HandleEntry::Type::Light:
      model.lights.push_back(&lights[entry->second.index]);
      break;
    case HandleEntry::Type::Model:
      // ignore child models
      break;
    default:
      char buffer[256];
      throw Error(format(buffer, "Unexpected child object type for model '%.*s' : '%d'", model.name.len, model.name.data, (int)entry->second.type));
    }
  }

  void connectMaterial(FbxMaterial& material, uint32_t child)
  {
    auto entry = table.find(child);
    enforce(entry != table.end(), "Unexisting child handle for material '%.*s' : '%.8X'", material.name.len, material.name.data, child);
    switch(entry->second.type)
    {
    case HandleEntry::Type::Texture:
      material.texture = &textures[entry->second.index];
      break;
    default:
      char buffer[256];
      throw Error(format(buffer, "Unexpected child object type for material '%.*s' : '%d'", material.name.len, material.name.data, (int)entry->second.type));
    }
  }

  ImportedScene finish()
  {
    ImportedScene scene {};

    for(auto& model : models)
    {
      if(model.lights.size())
      {
        auto pos = model.transform * Vector4f{ 0, 0, 0, 1 };
        Light light;
        light.x = pos.x;
        light.y = pos.y;
        light.z = pos.z;
        light.r = model.lights[0]->rgb.x * model.lights[0]->intensity;
        light.g = model.lights[0]->rgb.y * model.lights[0]->intensity;
        light.b = model.lights[0]->rgb.z * model.lights[0]->intensity;
        scene.lights.push_back(light);
      }
      else
      {
        enforce(model.materials.size() == 1, "Invalid material count for model '%.*s': %d", model.name.len, model.name.data, model.materials.size());

        Mesh mesh;
        mesh.name.assign(model.name.data, model.name.len);

        if(FbxTexture* tex = model.materials[0]->texture)
          mesh.material.assign(tex->filename.data, tex->filename.len);

        enforce(model.geometry.size() <= 1, "Too many meshes for model '%.*s'", model.name.len, model.name.data, model.materials.size());

        for(auto& geom : model.geometry)
        {
          mesh.vertices = geom->vertices;
          mesh.faces = geom->faces;
          mesh.transform = model.transform;
        }

        scene.meshes.push_back(std::move(mesh));
      }
    }

    return scene;
  }

  void parseSection_Objects(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    skipValues(tokenizer);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      case hashed("Geometry"):
        parseSection_Geometry(tokenizer);
        break;
      case hashed("Model"):
        parseSection_Model(tokenizer);
        break;
      case hashed("Material"):
        parseSection_Material(tokenizer);
        break;
      case hashed("NodeAttribute"):
        parseSection_NodeAttribute(tokenizer);
        break;
      case hashed("Texture"):
        parseSection_Texture(tokenizer);
        break;
      case hashed("Video"):
        skipObject(tokenizer);
        break;
      default:
        char buffer[256];
        throw Error(format(buffer, "Unexpected child object '%.*s' in 'Objects'", tokenizer.objName.len, tokenizer.objName.data));
      }
    }

    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_Connections(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      case hashed("C"):
        parseSection_C(tokenizer);
        break;
      default:
        char buffer[256];
        throw Error(format(buffer, "Unexpected child object '%.*s' in 'Objects'", tokenizer.objName.len, tokenizer.objName.data));
      }
    }

    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_C(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    auto type = expectString(tokenizer);
    switch(hashed(type))
    {
    case hashed("OO"):
      {
        const auto child = expectInteger(tokenizer);
        const auto parent = expectInteger(tokenizer);
        m_connections[parent].push_back(child);
        break;
      }
    case hashed("OP"):
      {
        const auto child = expectInteger(tokenizer);
        const auto parent = expectInteger(tokenizer);
        const auto ctype = expectString(tokenizer);
        m_connections[parent].push_back(child);
        (void)ctype;
        break;
      }
    case hashed("PP"):
      break;
    default:
      char buffer[256];
      throw Error(format(buffer, "Unknown connection type: %.*s", type.len, type.data));
    }

    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_Geometry(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    FbxGeometry node {};
    node.handle = expectInteger(tokenizer);
    node.name = expectString(tokenizer);

    skipValues(tokenizer);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objectName = expectObjectBegin(tokenizer);
      switch(hashed(objectName))
      {
      case hashed("Vertices"):
        parseSection_Vertices(tokenizer);
        break;
      case hashed("PolygonVertexIndex"):
        parseSection_Indices(tokenizer);
        break;
      case hashed("LayerElementNormal"):
        parseSection_LayerElementNormal(tokenizer);
        break;
      case hashed("LayerElementUV"):
        parseSection_LayerElementUV(tokenizer);
        break;
      default:
        skipObject(tokenizer);
        break;
      }
    }

    expect(tokenizer, Token::ObjectEnd);
    flushGeometry(node);

    {
      const int index = (int)geometry.size();
      table[node.handle] = { HandleEntry::Type::Geometry, index };
      geometry.push_back(std::move(node));
    }
  }

  void parseSection_NodeAttribute(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    FbxLight light {};

    light.intensity = 1;
    light.rgb = { 1, 1, 1 };
    light.handle = expectInteger(tokenizer);
    /* auto name = */ expectString(tokenizer);
    auto type = expectString(tokenizer);

    skipValues(tokenizer);

    if(hashed(type) == hashed("Light"))
    {
      while(tokenizer.tokenType != Token::ObjectEnd)
      {
        const auto objName = expectObjectBegin(tokenizer);
        switch(hashed(objName))
        {
        case hashed("Properties70"):
          parseSection_Light_Properties70(tokenizer, light);
          break;
        default:
          skipObject(tokenizer);
          break;
        }
      }
    }
    else
    {
      while(tokenizer.tokenType != Token::ObjectEnd)
        skipObject(tokenizer);
    }

    expect(tokenizer, Token::ObjectEnd);

    {
      const int index = (int)lights.size();
      table[light.handle] = { HandleEntry::Type::Light, index };
      lights.push_back(std::move(light));
    }
  }

  void parseSection_Model(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    FbxModel model {};

    model.handle = expectInteger(tokenizer);
    model.name = expectString(tokenizer);
    model.transform = identity;

    skipValues(tokenizer);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      case hashed("Properties70"):
        parseSection_Model_Properties70(tokenizer, model);
        break;
      default:
        skipObject(tokenizer);
        break;
      }
    }

    expect(tokenizer, Token::ObjectEnd);

    {
      const int index = (int)models.size();
      table[model.handle] = { HandleEntry::Type::Model, index };
      models.push_back(std::move(model));
    }
  }

  void parseSection_Model_Properties70(Tokenizer& tokenizer, FbxModel& model)
  {
    Matrix4f translation = identity;
    Matrix4f rotation = identity;
    Matrix4f scaling = identity;

    expect(tokenizer, Token::ObjectBegin);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      enforce(hashed(tokenizer.objName) == hashed("P"), "Unexpected object '%.*s'", tokenizer.objName.len, tokenizer.objName.data);

      expect(tokenizer, Token::ObjectBegin);

      const auto propName = expectString(tokenizer);
      /* const auto propName2 = */ expectString(tokenizer);
      /* const auto someString = */ expectString(tokenizer);
      /* const auto theLetterA = */ expectString(tokenizer);
      switch(hashed(propName))
      {
      case hashed("Lcl Translation"):
        {
          double x = expectDecimalNumber(tokenizer) / 100;
          double y = expectDecimalNumber(tokenizer) / 100;
          double z = expectDecimalNumber(tokenizer) / 100;
          translation = ::translate(Vector3f(x, y, z));
        }
        break;
      case hashed("Lcl Rotation"):
        {
          double x = expectDecimalNumber(tokenizer) * PI / 180.0;
          double y = expectDecimalNumber(tokenizer) * PI / 180.0;
          double z = expectDecimalNumber(tokenizer) * PI / 180.0;
          rotation = ::rotateX(x);
          rotation = ::rotateY(y) * rotation;
          rotation = ::rotateZ(z) * rotation;
        }
        break;
      case hashed("Lcl Scaling"):
        {
          double x = expectDecimalNumber(tokenizer) / 100;
          double y = expectDecimalNumber(tokenizer) / 100;
          double z = expectDecimalNumber(tokenizer) / 100;
          scaling = ::scale(Vector3f(x, y, z));
        }
        break;
      }

      skipValues(tokenizer);

      expect(tokenizer, Token::ObjectEnd);
    }

    expect(tokenizer, Token::ObjectEnd);

    model.transform = translation * rotation * scaling;
  }

  void parseSection_Light_Properties70(Tokenizer& tokenizer, FbxLight& light)
  {
    expect(tokenizer, Token::ObjectBegin);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      enforce(hashed(tokenizer.objName) == hashed("P"), "Unexpected object '%.*s'", tokenizer.objName.len, tokenizer.objName.data);

      expect(tokenizer, Token::ObjectBegin);

      const auto propName = expectString(tokenizer);
      /* const auto propName2 = */ expectString(tokenizer);
      /* const auto someString = */ expectString(tokenizer);
      /* const auto theLetterA = */ expectString(tokenizer);
      switch(hashed(propName))
      {
      case hashed("Color"):
        {
          light.rgb.x = expectDecimalNumber(tokenizer);
          light.rgb.y = expectDecimalNumber(tokenizer);
          light.rgb.z = expectDecimalNumber(tokenizer);
        }
        break;
      case hashed("Intensity"):
        {
          light.intensity = expectDecimalNumber(tokenizer) / 10000;
        }
        break;
      case hashed("CastLight"):
      case hashed("DecayStart"):
      case hashed("DecayType"):
      case hashed("LightType"):
      case hashed("AreaLightShape"):
      case hashed("CastShadows"):
      case hashed("ShadowColor"):
        {
          while(tokenizer.tokenType == Token::Value)
            tokenizer.nextToken();
        }
        break;
      default:
        {
          char buffer[256];
          throw Error(format(buffer, "Unexpected property for light : '%.*s'", propName.len, propName.data));
        }
      }

      expect(tokenizer, Token::ObjectEnd);
    }

    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_Material(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    FbxMaterial node {};
    node.handle = expectInteger(tokenizer);
    node.name = expectString(tokenizer);

    skipValues(tokenizer);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      default:
        skipObject(tokenizer);
        break;
      }
    }

    expect(tokenizer, Token::ObjectEnd);

    {
      const int index = (int)materials.size();
      table[node.handle] = { HandleEntry::Type::Material, index };
      materials.push_back(std::move(node));
    }
  }

  void parseSection_Texture(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    FbxTexture node {};
    node.handle = expectInteger(tokenizer);
    node.name = expectString(tokenizer);
    node.filename = "?";

    skipValues(tokenizer);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      case hashed("FileName"):
        expect(tokenizer, Token::ObjectBegin);
        node.filename = baseName(expectString(tokenizer));
        expect(tokenizer, Token::ObjectEnd);
        break;
      default:
        skipObject(tokenizer);
        break;
      }
    }

    expect(tokenizer, Token::ObjectEnd);

    {
      const int index = (int)textures.size();
      table[node.handle] = { HandleEntry::Type::Texture, index };
      textures.push_back(std::move(node));
    }
  }

  void parseSection_LayerElementNormal(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    skipValues(tokenizer);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      case hashed("Normals"):
        parseSection_Normals(tokenizer);
        break;
      default:
        skipObject(tokenizer);
        break;
      }
    }

    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_LayerElementUV(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    skipValues(tokenizer);

    while(tokenizer.tokenType != Token::ObjectEnd)
    {
      const auto objName = expectObjectBegin(tokenizer);
      switch(hashed(objName))
      {
      case hashed("UV"):
        {
          expect(tokenizer, Token::ObjectBegin);

          enforce(tokenizer.tokenType == Token::Value, "No values in 'UV'");
          enforce(tokenizer.valueType == ValueType::DoubleArray, "Unexpected value (type: %d) in 'UV'", tokenizer.valueType);

          auto& doubles = tokenizer.m_valueDoubleArray;
          enforce(doubles.len % 2 == 0, "Unexpected double count (%d) in 'UV'", doubles.len);

          m_vertexUv.reserve(doubles.len / 2);

          for(int i = 0; i < doubles.len; i += 2)
            m_vertexUv.push_back(Vector2f(doubles[i], doubles[i + 1]));

          tokenizer.nextToken();

          expect(tokenizer, Token::ObjectEnd);
        }
        break;
      case hashed("UVIndex"):
        {
          expect(tokenizer, Token::ObjectBegin);

          enforce(tokenizer.tokenType == Token::Value, "No values in 'UVIndex'");
          enforce(tokenizer.valueType == ValueType::IntegerArray, "Unexpected value (type: %d) in 'UVIndex'", tokenizer.valueType);

          auto& ints = tokenizer.m_valueIntegerArray;
          enforce(ints.len % 2 == 0, "Unexpected double count (%d) in 'UVIndex'", ints.len);

          m_vertexUvIndices.assign(ints.data, ints.data + ints.len);

          tokenizer.nextToken();

          expect(tokenizer, Token::ObjectEnd);
        }
        break;
      default:
        skipObject(tokenizer);
        break;
      }
    }

    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_Vertices(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    enforce(tokenizer.tokenType == Token::Value, "No values in 'Vertices'");
    enforce(tokenizer.valueType == ValueType::DoubleArray, "Unexpected value (type: %d) in 'Vertices'", tokenizer.valueType);

    auto& doubles = tokenizer.m_valueDoubleArray;
    enforce(doubles.len % 3 == 0, "Unexpected double count (%d) in 'Vertices'", doubles.len);

    m_vertexPos.reserve(doubles.len / 3);

    for(int i = 0; i < doubles.len; i += 3)
    {
      const float x = doubles[i + 0];
      const float y = doubles[i + 1];
      const float z = doubles[i + 2];
      m_vertexPos.push_back({ x, y, z });
    }

    tokenizer.nextToken();

    enforce(tokenizer.tokenType == Token::ObjectEnd, "Unexpected extra child/value in 'Vertices'");
    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_Indices(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    enforce(tokenizer.tokenType == Token::Value, "No values in 'Indices'");
    enforce(tokenizer.valueType == ValueType::IntegerArray, "Unexpected value (type: %d) in 'Indices'", tokenizer.valueType);

    auto ints = tokenizer.m_valueIntegerArray;

    m_faceIndices.reserve(ints.len);

    for(auto idx : tokenizer.m_valueIntegerArray)
      m_faceIndices.push_back(idx);

    tokenizer.nextToken();

    expect(tokenizer, Token::ObjectEnd);
  }

  void parseSection_Normals(Tokenizer& tokenizer)
  {
    expect(tokenizer, Token::ObjectBegin);

    enforce(tokenizer.tokenType == Token::Value, "No values in 'Normals'");
    enforce(tokenizer.valueType == ValueType::DoubleArray, "Unexpected value (type: %d) in 'Normals'", tokenizer.valueType);

    auto& doubles = tokenizer.m_valueDoubleArray;

    enforce(doubles.len % 3 == 0, "Unexpected double count (%d) in 'Normals'", doubles.len);

    m_vertexPos.reserve(doubles.len / 3);

    for(int i = 0; i < doubles.len; i += 3)
    {
      const float x = doubles[i + 0];
      const float y = doubles[i + 1];
      const float z = doubles[i + 2];
      m_vertexNormals.push_back({ x, y, z });
    }

    tokenizer.nextToken();

    enforce(tokenizer.tokenType == Token::ObjectEnd, "Unexpected extra child/value in 'Normals'");

    expect(tokenizer, Token::ObjectEnd);
  }

  void flushGeometry(FbxGeometry& node)
  {
    if(!m_faceIndices.empty())
    {
      static auto toVertex = [] (Vector3f pos, Vector3f n, Vector2f uv)
        {
          return Mesh::Vertex { pos.x, pos.y, pos.z, n.x, n.y, n.z, uv.x, uv.y };
        };

      auto indices = Span<int>(m_faceIndices);
      auto normals = Span<Vector3f>(m_vertexNormals);

      assert(normals.len == indices.len);

      int i = 0;

      while(i < indices.len)
      {
        // one iteration = one input face

        const int firstIt = i;
        const int firstIndex = indices[i++];
        int prevIt = i;
        int prevIndex = indices[i++];

        while(i < indices.len)
        {
          // one iteration = one output triangle
          const int currIt = i;
          const int currIndex = indices[i++];

          const int k0 = firstIndex;
          const int k1 = prevIndex;
          const int k2 = currIndex >= 0 ? currIndex : ~currIndex;

          {
            const int f0 = (int)node.vertices.size();
            node.vertices.push_back(toVertex(m_vertexPos.at(k0), m_vertexNormals.at(firstIt), m_vertexUv[m_vertexUvIndices[firstIt]]));
            node.vertices.push_back(toVertex(m_vertexPos.at(k1), m_vertexNormals.at(prevIt), m_vertexUv[m_vertexUvIndices[prevIt]]));
            node.vertices.push_back(toVertex(m_vertexPos.at(k2), m_vertexNormals.at(currIt), m_vertexUv[m_vertexUvIndices[currIt]]));

            node.faces.push_back({ f0, f0 + 1, f0 + 2 });
          }

          if(currIndex < 0)
            break;

          prevIndex = currIndex;
          prevIt = currIt;
        }
      }
    }

    m_vertexPos.clear();
    m_vertexNormals.clear();
    m_faceIndices.clear();
    m_vertexUv.clear();
    m_vertexUvIndices.clear();
  }

  // intermediate data
  std::vector<Vector3f> m_vertexPos;
  std::vector<Vector3f> m_vertexNormals;
  std::vector<Vector2f> m_vertexUv;
  std::vector<int> m_vertexUvIndices;
  std::vector<int> m_faceIndices;

  struct FbxNode
  {
    uint32_t handle;
    String name;
  };

  struct FbxTexture : FbxNode
  {
    String filename;
  };

  struct FbxMaterial : FbxNode
  {
    // only valid after 'Connections' pass
    FbxTexture* texture {};
  };

  struct FbxGeometry : FbxNode
  {
    std::vector<Mesh::Vertex> vertices;
    std::vector<Mesh::Face> faces;
  };

  struct FbxLight : FbxNode
  {
    Vector3f rgb;
    float intensity;
  };

  struct FbxModel : FbxNode
  {
    Matrix4f transform;

    // only valid after 'Connections' pass
    std::vector<FbxGeometry*> geometry;
    std::vector<FbxMaterial*> materials;
    std::vector<FbxLight*> lights;
  };

  /////////////////////////////////////////////////////////////////////////////
  // List of all nodes, by type
  std::vector<FbxModel> models;
  std::vector<FbxMaterial> materials;
  std::vector<FbxTexture> textures;
  std::vector<FbxGeometry> geometry;
  std::vector<FbxLight> lights;

  /////////////////////////////////////////////////////////////////////////////
  // List of all nodes, by handle value
  struct HandleEntry
  {
    enum class Type { Unknown, Model, Material, Texture, Geometry, Light, };
    Type type;
    int index; // index in above vectors
  };

  std::map<uint32_t, HandleEntry> table; // [handle] -> HandleEntry

  /////////////////////////////////////////////////////////////////////////////
  // Input to the 'Connections' pass:
  // Connections: [parent] -> { children }
  std::map<uint32_t, std::vector<uint32_t>> m_connections;
};

void parseFbxFileHeader(Span<const uint8_t>& data)
{
  static const uint8_t signatureData[] = "Kaydara FBX Binary  \x00\x1A";
  static const Span<const uint8_t> signature(signatureData);

  enforce(data.len >= signature.len, "FBX data is too small");
  enforce(memcmp(data.data, signature.data, signature.len) == 0, "Invalid FBX signature");

  data += signature.len;

  const uint32_t version = parseUint32(data);
  enforce(version <= 7400, "Unsupported FBX version: %d", version);
}
}

ImportedScene parseFbx(Span<const uint8_t> data)
{
  const uint8_t* const base = data.data; // used to convert endOffset to sizes

  parseFbxFileHeader(data);

  Tokenizer tokenizer(data, base);
  return FbxParser().parse(tokenizer);
}

