#ifndef SCRIPT_H_INCLUDED
#define SCRIPT_H_INCLUDED

#include <cstdint>
#include <memory>
#include "stream.h"
#include "mesh.h"
#include <unordered_map>
#include "client.h"
#include <sstream>

using namespace std;

class Script final : public enable_shared_from_this<Script>
{
public:
    struct Data : public enable_shared_from_this<Data>
    {
        enum class Type : uint8_t
        {
            Integer,
            Float,
            Vector,
            Matrix,
            List,
            Object,
            String,
            Last
        };
        static void writeType(Writer &writer, Type type)
        {
            writer.writeU8((uint8_t)type);
        }
        static Type readType(Reader &reader)
        {
            return (Type)reader.readLimitedU8(0, (uint8_t)Type::Last);
        }
        virtual Type type() const = 0;
        virtual ~Data()
        {
        }
        virtual shared_ptr<Data> dup() const = 0;
        virtual void write(Writer &writer) const = 0;
        static shared_ptr<Data> read(Reader &reader);
        virtual explicit operator wstring() const = 0;
        wstring typeString() const
        {
            switch(type())
            {
            case Integer:
                return L"integer";
            case Float:
                return L"float";
            case Vector:
                return L"vector";
            case Matrix:
                return L"matrix";
            case List:
                return L"list";
            case Object:
                return L"object";
            case String:
                return L"string";
            }
            assert(false);
            return L"unknown";
        }
        struct Integer final : public Data
        {
            virtual Type type() const override
            {
                return Type::Integer;
            }
            uint_fast32_t value;
            Integer(uint_fast32_t value = 0)
                : value((uint32_t)value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                return shared_ptr<Data>(new Integer(value));
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                writer.writeS32(value);
            }
            friend class Data;
        private:
            static shared_ptr<Integer> read(Reader &reader)
            {
                return make_shared<Integer>(reader.readS32());
            }
        public:
            virtual explicit operator wstring() const override
            {
                wostringstream os;
                os << value;
                return os.str();
            }
        };
        struct Float final : public Data
        {
            virtual Type type() const override
            {
                return Type::Float;
            }
            float value;
            Float(float value = 0)
                : value(value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                return shared_ptr<Data>(new Float(value));
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                writer.writeF32(value);
            }
            friend class Data;
        private:
            static shared_ptr<Float> read(Reader &reader)
            {
                return make_shared<Float>(reader.readFiniteF32());
            }
        public:
            virtual explicit operator wstring() const override
            {
                wostringstream os;
                os << value;
                return os.str();
            }
        };
        struct Vector final : public Data
        {
            virtual Type type() const override
            {
                return Type::Vector;
            }
            VectorF value;
            Vector(VectorF value = VectorF(0))
                : value(value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                return shared_ptr<Data>(new Vector(value));
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                writer.writeF32(value.x);
                writer.writeF32(value.y);
                writer.writeF32(value.z);
            }
            friend class Data;
        private:
            static shared_ptr<Vector> read(Reader &reader)
            {
                VectorF value;
                value.x = reader.readFiniteF32();
                value.y = reader.readFiniteF32();
                value.z = reader.readFiniteF32();
                return make_shared<Vector>(value);
            }
        public:
            virtual explicit operator wstring() const override
            {
                wostringstream os;
                os << L"<" << value.x << L", " << value.y << L", " << value.z << L">";
                return os.str();
            }
        };
        struct Matrix final : public Data
        {
            virtual Type type() const override
            {
                return Type::Matrix;
            }
            ::Matrix value;
            Matrix(::Matrix value = ::Matrix::identity())
                : value(value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                return shared_ptr<Data>(new Matrix(value));
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                for(int x = 0; x < 4; x++)
                    for(int y = 0; y < 3; y++)
                        writer.writeF32(value.get(x, y));
            }
            friend class Data;
        private:
            static shared_ptr<Matrix> read(Reader &reader)
            {
                ::Matrix value;
                for(int x = 0; x < 4; x++)
                    for(int y = 0; y < 3; y++)
                        value.set(x, y, reader.readFiniteF32());
                return make_shared<Matrix>(value);
            }
        public:
            virtual explicit operator wstring() const override
            {
                wostringstream os;
                for(int y = 0; y < 4; y++)
                {
                    const wchar_t * str = L"|";
                    for(int x = 0; x < 4; x++)
                    {
                        os << str << value.get(x, y);
                        str = L" ";
                    }
                    os << L"|\n";
                }
                return os.str();
            }
        };
        struct List final : public Data
        {
            virtual Type type() const override
            {
                return Type::List;
            }
            vector<shared_ptr<Data>> value;
            List(const vector<shared_ptr<Data>> & value = vector<shared_ptr<Data>>())
                : value(value)
            {
            }
            List(vector<shared_ptr<Data>> && value)
                : value(value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                auto retval = shared_ptr<List>(new List);
                retval->value.reserve(value.size());
                for(shared_ptr<Data> e : value)
                {
                    retval->value.push_back(e->dup());
                }
                return static_pointer_cast<Data>(retval);
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                assert((uint32_t)value.size() == value.size() && value.size() != (uint32_t)-1);
                writer.writeU32((uint32_t)value.size());
                for(shared_ptr<Data> v : value)
                {
                    assert(v);
                    v->write(writer);
                }
            }
            friend class Data;
        private:
            static shared_ptr<List> read(Reader &reader)
            {
                shared_ptr<List> retval = make_shared<List>();
                size_t length = reader.readLimitedU32(0, (uint32_t)-2);
                retval->value.reserve(length);
                for(size_t i = 0; i < length; i++)
                {
                    retval->value.push_back(Data::read(reader));
                }
                return retval;
            }
        public:
            virtual explicit operator wstring() const override
            {
                if(value.size() == 0)
                    return L"[]";
                wostringstream os;
                wchar_t ch = L'[';
                for(shared_ptr<Data> e : value)
                {
                    os << ch << (wstring)*e;
                    ch = L' ';
                }
                os << L"]";
                return os.str();
            }
        };
        struct Object final : public Data
        {
            virtual Type type() const override
            {
                return Type::Object;
            }
            unordered_map<wstring, shared_ptr<Data>> value;
            Object(const unordered_map<wstring, shared_ptr<Data>> & value = unordered_map<wstring, shared_ptr<Data>>())
                : value(value)
            {
            }
            Object(unordered_map<wstring, shared_ptr<Data>> && value)
                : value(value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                auto retval = shared_ptr<Object>(new Object);
                for(pair<wstring, shared_ptr<Data>> e : value)
                {
                    retval->value.push_back(make_pair(get<0>(e), get<1>(e)->dup());
                }
                return static_pointer_cast<Data>(retval);
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                assert((uint32_t)value.size() == value.size() && value.size() != (uint32_t)-1);
                writer.writeU32((uint32_t)value.size());
                for(pair<wstring, shared_ptr<Data>> v : value)
                {
                    assert(get<1>(v));
                    writer.writeString(get<0>(v));
                    get<1>(v)->write(writer);
                }
            }
            friend class Data;
        private:
            static shared_ptr<Object> read(Reader &reader)
            {
                shared_ptr<Object> retval = make_shared<Object>();
                size_t length = reader.readLimitedU32(0, (uint32_t)-2);
                for(size_t i = 0; i < length; i++)
                {
                    wstring name = reader.readString();
                    retval->value[name] = Data::read(reader);
                }
                return retval;
            }
        public:
            virtual explicit operator wstring() const override
            {
                if(value.size() == 0)
                    return L"{}";
                wostringstream os;
                wchar_t ch = L'{';
                for(pair<wstring, shared_ptr<Data>> e : value)
                {
                    os << ch << L"\"" << get<0>(e) << L"\" = " << (wstring)*get<1>(e);
                    ch = L' ';
                }
                os << L"}";
                return os.str();
            }
        };
        struct String final : public Data
        {
            virtual Type type() const override
            {
                return Type::String;
            }
            wstring value;
            String(wstring value = L"")
                : value(value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                return shared_ptr<Data>(new String(value));
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                writer.writeString(value);
            }
            friend class Data;
        private:
            static shared_ptr<String> read(Reader &reader)
            {
                return make_shared<String>(reader.readString());
            }
        public:
            virtual explicit operator wstring() const override
            {
                return value;
            }
        };
    };
    struct ScriptException final : public exception
    {
        shared_ptr<Data> data;
        string ndata;
        ScriptException(shared_ptr<Data> data)
            : data(data), ndata(wcsrtombs((wstring)*data))
        {
        }
        virtual const char* what() const override
        {
            return ndata.c_str();
        }
    };
    class State;
    struct Node : public enable_shared_from_this<Node>
    {
        enum class Type : uint_fast16_t
        {
            CastToString,
            CastToInteger,
            CastToFloat,
            CastToVector,
            CastToMatrix,
            Last
        };

        static Type readType(Reader &reader)
        {
            return (Type)reader.readLimitedU16(0, (uint16_t)Type::Last);
        }

        static void writeType(Writer &writer, Type type)
        {
            writer.writeU16((uint16_t)type);
        }

        virtual ~Node()
        {
        }
        virtual Type type() const = 0;
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth = 0) const = 0;
        virtual void write(Writer &writer) const = 0;
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount);
    };
    struct State final
    {
        shared_ptr<Object> variables;
        const vector<shared_ptr<Node>> & nodes;
        State(const vector<shared_ptr<Node>> & nodes)
            : variables(make_shared<Object>()), nodes(nodes)
        {
        }
    };
    template<uint32_t size>
    struct NodeConstArgCount : public Node
    {
        friend class Node;
        uint32_t args[size];
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            for(uint32_t & v : args)
            {
                v = reader.readLimitedU32(0, nodeCount - 1);
            }
        }
    public:
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            for(uint32_t v : args)
            {
                writer.writeU32(v);
            }
        }
    };
    struct NodeCastToString final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToString;
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            shared_ptr<Data> retval = state.nodes[arg[0]]->evaluate(state, stackDepth + 1);
            return make_shared<Data::String>((wstring)*retval);
        }
    };
    struct NodeCastToInteger final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToInteger;
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            shared_ptr<Data> retval = state.nodes[arg[0]]->evaluate(state, stackDepth + 1);
            if(retval->type() == Data::Type::Integer)
                return retval;
            if(retval->type() == Data::Type::Float)
                return make_shared<Data::Integer>(dynamic_pointer_cast<Data::Float>(retval)->value);
            throw ScriptException(make_shared<Data::String>(L"type cast error : can't cast " + retval->typeString() + " to integer"));
        }
    };
    struct NodeCastToFloat final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToFloat;
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            shared_ptr<Data> retval = state.nodes[arg[0]]->evaluate(state, stackDepth + 1);
            if(retval->type() == Data::Type::Float)
                return retval;
            if(retval->type() == Data::Type::Integer)
                return make_shared<Data::Float>(dynamic_pointer_cast<Data::Integer>(retval)->value);
            throw ScriptException(make_shared<Data::String>(L"type cast error : can't cast " + retval->typeString() + " to float"));
        }
    };
    struct NodeCastToVector final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToVector;
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            shared_ptr<Data> retval = state.nodes[arg[0]]->evaluate(state, stackDepth + 1);
            if(retval->type() == Data::Type::Vector)
                return retval;
            if(retval->type() == Data::Type::Integer)
                return make_shared<Data::Vector>(VectorF(dynamic_pointer_cast<Data::Integer>(retval)->value));
            if(retval->type() == Data::Type::Float)
                return make_shared<Data::Vector>(VectorF(dynamic_pointer_cast<Data::Float>(retval)->value));
            if(retval->type() == Data::Type::List)
            {
                #error finish
            }
            throw ScriptException(make_shared<Data::String>(L"type cast error : can't cast " + retval->typeString() + " to vector"));
        }
    };
    #error change matrix to only cast from list
    struct NodeCastToMatrix final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToMatrix;
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            shared_ptr<Data> retval = state.nodes[arg[0]]->evaluate(state, stackDepth + 1);
            if(retval->type() == Data::Type::Matrix)
                return retval;
            if(retval->type() == Data::Type::Integer)
                return make_shared<Data::Matrix>(Matrix::scale(dynamic_pointer_cast<Data::Integer>(retval)->value));
            if(retval->type() == Data::Type::Float)
                return make_shared<Data::Matrix>(Matrix::scale(dynamic_pointer_cast<Data::Float>(retval)->value));
            if(retval->type() == Data::Type::Vector)
                return make_shared<Data::Matrix>(Matrix::scale(dynamic_pointer_cast<Data::Vector>(retval)->value));
            throw ScriptException(make_shared<Data::String>(L"type cast error : can't cast " + retval->typeString() + " to matrix"));
        }
    };
    vector<shared_ptr<Node>> nodes;
    shared_ptr<Data> evaluate() const
    {
        State state(nodes);
        return nodes.front()->evaluate(state);
    }
    Matrix evaluateAsMatrix() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Matrix)
            throw ScriptException(retval);
        return dynamic_pointer_cast<Data::Matrix>(retval)->value;
    }
    VectorF evaluateAsVector() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Vector)
            throw ScriptException(retval);
        return dynamic_pointer_cast<Data::Vector>(retval)->value;
    }
    int32_t evaluateAsInteger() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Integer)
            throw ScriptException(retval);
        return dynamic_pointer_cast<Data::Integer>(retval)->value;
    }
    float evaluateAsFloat() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Float)
            throw ScriptException(retval);
        return dynamic_pointer_cast<Data::Float>(retval)->value;
    }
    wstring evaluateAsString() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::String)
            throw ScriptException(retval);
        return dynamic_pointer_cast<Data::String>(retval)->value;
    }
};

inline shared_ptr<Script::Data> Script::Data::read(Reader &reader)
{
    Type type = readType(reader);
    switch(type)
    {
    case Type::Integer:
        return static_pointer_cast<Data>(Integer::read(reader));
    case Type::Float:
        return static_pointer_cast<Data>(Float::read(reader));
    case Type::Vector:
        return static_pointer_cast<Data>(Vector::read(reader));
    case Type::Matrix:
        return static_pointer_cast<Data>(Matrix::read(reader));
    case Type::List:
        return static_pointer_cast<Data>(List::read(reader));
    case Type::Object:
        return static_pointer_cast<Data>(Object::read(reader));
    case Type::String:
        return static_pointer_cast<Data>(String::read(reader));
    case Type::Last:
        break;
    }
    assert(false);
}

inline shared_ptr<Script::Node> Script::Node::read(Reader &reader)
{

}

#endif // SCRIPT_H_INCLUDED
