#ifndef SCRIPT_H_INCLUDED
#define SCRIPT_H_INCLUDED

#include <cstdint>
#include <memory>
#include "stream.h"
#include "mesh.h"
#include <unordered_map>
#include "client.h"
#include <sstream>
#include <cmath>

using namespace std;

class Script final : public enable_shared_from_this<Script>
{
public:
    struct Data : public enable_shared_from_this<Data>
    {
        enum class Type : uint8_t
        {
            Boolean,
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
        struct Boolean final : public Data
        {
            virtual Type type() const override
            {
                return Type::Boolean;
            }
            bool value;
            Boolean(bool value = false)
                : value(value)
            {
            }
            virtual shared_ptr<Data> dup() const override
            {
                return shared_ptr<Data>(new Boolean(value));
            }
            virtual void write(Writer &writer) const override
            {
                writeType(writer, type());
                writer.writeBool(value);
            }
            friend class Data;
        private:
            static shared_ptr<Boolean> read(Reader &reader)
            {
                return make_shared<Boolean>(reader.readBool());
            }
        public:
            virtual explicit operator wstring() const override
            {
                if(value)
                    return L"true";
                return L"false";
            }
        };
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
        ScriptException(wstring str)
            : ScriptException(static_pointer_cast<Data>(make_shared<Data::String>(str)))
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
            Const,
            CastToString,
            CastToInteger,
            CastToFloat,
            CastToVector,
            CastToMatrix,
            CastToList,
            LoadGlobals,
            ReadIndex,
            AssignIndex,
            Add,
            Sub,
            Mul,
            Div,
            Mod,
            Pow,
            And,
            Or,
            Xor,
            Concat,
            Dot,
            Cross,
            Equal,
            NotEqual,
            LessThan,
            GreaterThan,
            LessEqual,
            GreaterEqual,
            Not,
            Abs,
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
    protected:
        static void checkStackDepth(unsigned stackDepth)
        {
            if(stackDepth > 1000)
                throw ScriptException(L"stack depth limit exceeded");
        }
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
    struct NodeConst final : public Node
    {
        friend class Node;
        shared_ptr<Data> data;
        NodeConst(shared_ptr<Data> data)
            : data(data)
        {
        }
        virtual Type type() const override
        {
            return Type::Const;
        }
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t)
        {
            return make_shared<NodeConst>(Data::read(reader));
        }
    public:
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            data->write(writer);
        }
        virtual shared_ptr<Data> evaluate(State &, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return data;
        }
    };
    struct NodeLoadGlobals final : public Node
    {
        friend class Node;
        virtual Type type() const override
        {
            return Type::LoadGlobals;
        }
    protected:
        static shared_ptr<Node> read(Reader &, uint32_t)
        {
            return make_shared<NodeLoadGlobals>();
        }
    public:
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return state.variables;
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
            checkStackDepth(stackDepth);
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
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Integer)
                return retval;
            if(retval->type() == Data::Type::Float)
                return make_shared<Data::Integer>(dynamic_pointer_cast<Data::Float>(retval)->value);
            throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to integer");
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeCastToFloat final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToFloat;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
                return retval;
            if(retval->type() == Data::Type::Integer)
                return make_shared<Data::Float>(dynamic_pointer_cast<Data::Integer>(retval)->value);
            throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to float");
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeCastToVector final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToVector;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Vector)
                return retval;
            if(retval->type() == Data::Type::Integer)
                return make_shared<Data::Vector>(VectorF(dynamic_pointer_cast<Data::Integer>(retval)->value));
            if(retval->type() == Data::Type::Float)
                return make_shared<Data::Vector>(VectorF(dynamic_pointer_cast<Data::Float>(retval)->value));
            if(retval->type() == Data::Type::List)
            {
                Data::List * list = dynamic_cast<Data::List *>(retval.get());
                if(list->value.size() != 3)
                    throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to vector");
                VectorF v;
                v.x = dynamic_pointer_cast<Data::Float>(NodeCastToFloat::evaluate(list->value[0]))->value;
                v.y = dynamic_pointer_cast<Data::Float>(NodeCastToFloat::evaluate(list->value[1]))->value;
                v.z = dynamic_pointer_cast<Data::Float>(NodeCastToFloat::evaluate(list->value[2]))->value;
                return shared_ptr<Data>(new Data::Vector(v));
            }
            throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to vector");
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeCastToMatrix final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToMatrix;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Matrix)
                return retval;
            if(retval->type() == Data::Type::List)
            {
                Data::List * list = dynamic_cast<Data::List *>(retval.get());
                if(list->value.size() != 16)
                    throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to matrix");
                Matrix v;
                for(int i = 0, y = 0; y < 4; y++)
                {
                    for(int x = 0; x < 4; x++, i++)
                    {
                        v.set(x, y, dynamic_pointer_cast<Data::Float>(NodeCastToFloat::evaluate(list->value[0]))->value);
                    }
                }
                return shared_ptr<Data>(new Data::Matrix(v));
            }
            throw ScriptException(make_shared<Data::String>(L"type cast error : can't cast " + retval->typeString() + " to matrix"));
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeCastToList final : public NodeConstArgCount<1>
    {
        virtual Type type() const override
        {
            return Type::CastToList;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::List)
                return retval;
            if(retval->type() == Data::Type::Vector)
            {
                VectorF value = dynamic_cast<Data::Vector *>(retval.get())->value;
                shared_ptr<Data::List> retval2 = make_shared<Data::List>();
                retval2->value.push_back(make_shared<Data::Float>(value.x));
                retval2->value.push_back(make_shared<Data::Float>(value.y));
                retval2->value.push_back(make_shared<Data::Float>(value.z));
                return static_pointer_cast<Data>(retval2);
            }
            if(retval->type() == Data::Type::Matrix)
            {
                Matrix value = dynamic_cast<Data::Matrix *>(retval.get())->value;
                shared_ptr<Data::List> retval2 = make_shared<Data::List>();
                for(int y = 0; y < 4; y++)
                {
                    for(int x = 0; x < 4; x++)
                    {
                        retval2->value.push_back(make_shared<Data::Float>(value.get(x, y)));
                    }
                }
                return static_pointer_cast<Data>(retval2);
            }
            throw ScriptException(make_shared<Data::String>(L"type cast error : can't cast " + retval->typeString() + " to list"));
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeReadIndex final : public NodeConstArgCount<2>
    {
        virtual Type type() const override
        {
            return Type::ReadIndex;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::Vector)
            {
                if(arg2->type() == Data::Type::Integer)
                {
                    switch(dynamic_cast<Data::Integer *>(arg2.get())->value)
                    {
                    case 0:
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.x));
                    case 1:
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.y));
                    case 2:
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.z));
                    default:
                        throw ScriptException(L"index out of range");
                    }
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    if(value == L"x")
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.x));
                    if(value == L"y")
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.y));
                    if(value == L"z")
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.z));
                    if(value == L"length")
                        return shared_ptr<Data>(new Data::Integer(3));
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::Matrix)
            {
                if(arg2->type() == Data::Type::Integer)
                {
                    uint32_t value = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    if(value < 0 || value >= 16)
                    {
                        throw ScriptException(L"index out of range");
                    }
                    return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Matrix *>(arg1.get())->value.get(value % 4, value / 4)));
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    if(value == L"length")
                        return shared_ptr<Data>(new Data::Integer(16));
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(retval->type() == Data::Type::List)
            {
                const vector<shared_ptr<Data>> & list = dynamic_cast<Data::List *>(retval.get())->value;
                if(arg2->type() == Data::Type::Integer)
                {
                    uint32_t value = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    if(value < 0 || value >= list.size())
                    {
                        throw ScriptException(L"index out of range");
                    }
                    return list[value];
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    if(value == L"length")
                        return shared_ptr<Data>(new Data::Integer(list.size()));
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(retval->type() == Data::Type::Object)
            {
                const unordered_map<wstring, shared_ptr<Data>> & map = dynamic_cast<Data::Object *>(retval.get())->value;
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    auto i = map.find(value);
                    if(i == map.end())
                        throw ScriptException(L"variable doesn't exist : " + value);
                    return get<1>(*i);
                }
                throw ScriptException(L"illegal type for index");
            }
            throw ScriptException(L"invalid type to index");
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1), state.nodes[arg[1]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeAssignIndex final : public NodeConstArgCount<3>
    {
        virtual Type type() const override
        {
            return Type::AssignIndex;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> arg1, shared_ptr<Data> arg2, shared_ptr<Data> arg3)
        {
            if(arg1->type() == Data::Type::Vector)
            {
                if(arg2->type() == Data::Type::Integer)
                {
                    int32_t value = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    if(value < 0 || value >= 3)
                        throw ScriptException(L"index out of range");
                    if(arg3->type() != Data::Type::Float)
                        throw ScriptException(L"can't assign " + arg3->typeString() + " to float");
                    float newValue = dynamic_cast<Data::Float *>(arg3.get())->value;
                    switch(value)
                    {
                    case 0:
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.x = newValue));
                    case 1:
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.y = newValue));
                    case 2:
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.z = newValue));
                    default:
                        assert(false);
                    }
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    if(value != L"x" && value != L"y" && value != L"z")
                        throw ScriptException(L"can't write to " + value);
                    if(arg3->type() != Data::Type::Float)
                        throw ScriptException(L"can't assign " + arg3->typeString() + " to a float");
                    float newValue = dynamic_cast<Data::Float *>(arg3.get())->value;
                    if(value == L"x")
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.x = newValue));
                    if(value == L"y")
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.y = newValue));
                    if(value == L"z")
                        return shared_ptr<Data>(new Data::Float(dynamic_cast<Data::Vector *>(arg1.get())->value.z = newValue));
                    if(value == L"length")
                        return shared_ptr<Data>(new Data::Integer(3));
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::Matrix)
            {
                if(arg2->type() == Data::Type::Integer)
                {
                    uint32_t value = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    if(arg3->type() != Data::Type::Float)
                        throw ScriptException(L"can't assign " + arg3->typeString() + " to a float");
                    float newValue = dynamic_cast<Data::Float *>(arg3.get())->value;
                    if(value < 0 || value >= 16)
                    {
                        throw ScriptException(L"index out of range");
                    }
                    dynamic_cast<Data::Matrix *>(arg1.get())->value.set(value % 4, value / 4, newValue);
                    return shared_ptr<Data>(new Data::Float(newValue));
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    throw ScriptException(L"can't write to " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(retval->type() == Data::Type::List)
            {
                const vector<shared_ptr<Data>> & list = dynamic_cast<Data::List *>(retval.get())->value;
                if(arg2->type() == Data::Type::Integer)
                {
                    uint32_t value = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    if(value < 0 || value >= list.size())
                    {
                        throw ScriptException(L"index out of range");
                    }
                    return list[value] = arg3->dup();
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    throw ScriptException(L"can't write to " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(retval->type() == Data::Type::Object)
            {
                const unordered_map<wstring, shared_ptr<Data>> & map = dynamic_cast<Data::Object *>(retval.get())->value;
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<Data::String *>(arg2.get())->value;
                    return map[value] = arg3->dup();
                }
                throw ScriptException(L"illegal type for index");
            }
            throw ScriptException(L"invalid type to index");
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1), state.nodes[arg[1]]->evaluate(state, stackDepth + 1), state.nodes[arg[2]]->evaluate(state, stackDepth + 1));
        }
    };
    template <typename ChildClass>
    struct NodeBinaryArithmatic : public NodeConstArgCount<2>
    {
        static shared_ptr<Data> toData(float v)
        {
            return shared_ptr<Data>(new Data::Float(v));
        }
        static shared_ptr<Data> toData(int32_t v)
        {
            return shared_ptr<Data>(new Data::Integer(v));
        }
        static shared_ptr<Data> toData(VectorF v)
        {
            return shared_ptr<Data>(new Data::Vector(v));
        }
        static shared_ptr<Data> toData(Matrix v)
        {
            return shared_ptr<Data>(new Data::Matrix(v));
        }
        static shared_ptr<Data> toData(bool v)
        {
            return shared_ptr<Data>(new Data::Boolean(v));
        }
        static int32_t throwError()
        {
            throw ScriptException(L"invalid types for " + ChildClass::operatorString());
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            throwError();
            return nullptr;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::Float)
            {
                auto v1 = dynamic_cast<Data::Float *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<Data::Float *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<Data::Vector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<Data::Matrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Integer)
            {
                auto v1 = dynamic_cast<Data::Integer *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<Data::Float *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<Data::Vector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<Data::Matrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Vector)
            {
                auto v1 = dynamic_cast<Data::Vector *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<Data::Float *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<Data::Vector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<Data::Matrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Matrix)
            {
                auto v1 = dynamic_cast<Data::Matrix *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<Data::Float *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<Data::Integer *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<Data::Vector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<Data::Matrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Boolean && arg2->type() == Data::Type::Boolean)
            {
                auto v1 = dynamic_cast<Data::Boolean *>(arg1.get())->value;
                auto v2 = dynamic_cast<Data::Boolean *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            return evaluateBackup(arg1, arg2);
        }
        virtual shared_ptr<Data> evaluate(State & state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[arg[0]]->evaluate(state, stackDepth + 1), state.nodes[arg[1]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeAdd : public NodeBinaryArithmatic<NodeAdd>
    {
        virtual Type type() const override
        {
            return Type::Add;
        }
        int32_t evalFn(bool, bool)
        {
            return throwError();
        }
        VectorF evalFn(VectorF a, VectorF b)
        {
            return a + b;
        }
        int32_t evalFn(float, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, float)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        float evalFn(float a, float b)
        {
            return a + b;
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            return a + b;
        }
    };
    struct NodeSub : public NodeBinaryArithmatic<NodeSub>
    {
        virtual Type type() const override
        {
            return Type::Sub;
        }
        int32_t evalFn(bool, bool)
        {
            return throwError();
        }
        VectorF evalFn(VectorF a, VectorF b)
        {
            return a - b;
        }
        int32_t evalFn(float, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, float)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        float evalFn(float a, float b)
        {
            return a - b;
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            return a - b;
        }
    };
    struct NodeMul : public NodeBinaryArithmatic<NodeMul>
    {
        virtual Type type() const override
        {
            return Type::Mul;
        }
        int32_t evalFn(bool, bool)
        {
            return throwError();
        }
        VectorF evalFn(VectorF a, VectorF b)
        {
            return a - b;
        }
        VectorF evalFn(float a, VectorF b)
        {
            return a * b;
        }
        VectorF evalFn(VectorF a, float b)
        {
            return a * b;
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        float evalFn(float a, float b)
        {
            return a * b;
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            return a * b;
        }
    };
    struct NodeDiv : public NodeBinaryArithmatic<NodeDiv>
    {
        virtual Type type() const override
        {
            return Type::Div;
        }
        int32_t evalFn(bool, bool)
        {
            return throwError();
        }
        VectorF evalFn(VectorF a, VectorF b)
        {
            if(b.x == 0 || b.y == 0 || b.z == 0)
                throw ScriptException(L"divide by zero");
            return a / b;
        }
        int32_t evalFn(float a, VectorF b)
        {
            return throwError();
        }
        VectorF evalFn(VectorF a, float b)
        {
            if(b == 0)
                throw ScriptException(L"divide by zero");
            return a / b;
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        float evalFn(float a, float b)
        {
            if(b == 0)
                throw ScriptException(L"divide by zero");
            return a / b;
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            if(b == 0)
                throw ScriptException(L"divide by zero");
            return a / b;
        }
    };
    struct NodeMod : public NodeBinaryArithmatic<NodeMod>
    {
        virtual Type type() const override
        {
            return Type::Mod;
        }
        int32_t evalFn(bool, bool)
        {
            return throwError();
        }
        int32_t evalFn(VectorF a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(float a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(VectorF a, float b)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        float evalFn(float a, float b)
        {
            if(b == 0)
                throw ScriptException(L"divide by zero");
            return fmod(a, b);
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            if(b == 0)
                throw ScriptException(L"divide by zero");
            return a % b;
        }
    };
    struct NodePow : public NodeBinaryArithmatic<NodePow>
    {
        virtual Type type() const override
        {
            return Type::Pow;
        }
        int32_t evalFn(bool, bool)
        {
            return throwError();
        }
        int32_t evalFn(VectorF a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(float a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(VectorF a, float b)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        float evalFn(float a, float b)
        {
            if(b != (int)b && a <= 0)
                throw ScriptException(L"domain error");
            if(b <= 0 && a <= 0)
                throw ScriptException(L"domain error");
            return pow(a, b);
        }
    };
    struct NodeAnd : public NodeBinaryArithmatic<NodeAnd>
    {
        virtual Type type() const override
        {
            return Type::And;
        }
        bool evalFn(bool a, bool b)
        {
            return a && b;
        }
        int32_t evalFn(VectorF a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(float a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(VectorF a, float b)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(float a, float b)
        {
            return throwError();
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            return a & b;
        }
    };
    struct NodeOr : public NodeBinaryArithmatic<NodeOr>
    {
        virtual Type type() const override
        {
            return Type::Or;
        }
        bool evalFn(bool a, bool b)
        {
            return a || b;
        }
        int32_t evalFn(VectorF a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(float a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(VectorF a, float b)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(float a, float b)
        {
            return throwError();
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            return a | b;
        }
    };
    struct NodeXor : public NodeBinaryArithmatic<NodeXor>
    {
        virtual Type type() const override
        {
            return Type::Xor;
        }
        bool evalFn(bool a, bool b)
        {
            return a != b;
        }
        int32_t evalFn(VectorF a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(float a, VectorF b)
        {
            return throwError();
        }
        int32_t evalFn(VectorF a, float b)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, VectorF)
        {
            return throwError();
        }
        int32_t evalFn(Matrix, float)
        {
            return throwError();
        }
        int32_t evalFn(float, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(VectorF, Matrix)
        {
            return throwError();
        }
        int32_t evalFn(float a, float b)
        {
            return throwError();
        }
        int32_t evalFn(int32_t a, int32_t b)
        {
            return a ^ b;
        }
    };
    #error finish
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
    case Type::Boolean:
        return static_pointer_cast<Data>(Boolean::read(reader));
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
