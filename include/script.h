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
            case Type::Integer:
                return L"integer";
            case Type::Float:
                return L"float";
            case Type::Vector:
                return L"vector";
            case Type::Matrix:
                return L"matrix";
            case Type::List:
                return L"list";
            case Type::Object:
                return L"object";
            case Type::String:
                return L"string";
            case Type::Boolean:
                return L"boolean";
            case Type::Last:
                assert(false);
                return L"unknown";
            }
            assert(false);
            return L"unknown";
        }
    };
    struct DataBoolean final : public Data
    {
        virtual Type type() const override
        {
            return Type::Boolean;
        }
        bool value;
        DataBoolean(bool value = false)
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            return shared_ptr<Data>(new DataBoolean(value));
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            writer.writeBool(value);
        }
        friend class Data;
    private:
        static shared_ptr<DataBoolean> read(Reader &reader)
        {
            return make_shared<DataBoolean>(reader.readBool());
        }
    public:
        virtual explicit operator wstring() const override
        {
            if(value)
            {
                return L"true";
            }
            return L"false";
        }
    };
    struct DataInteger final : public Data
    {
        virtual Type type() const override
        {
            return Type::Integer;
        }
        int32_t value;
        DataInteger(int32_t value = 0)
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            return shared_ptr<Data>(new DataInteger(value));
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            writer.writeS32(value);
        }
        friend class Data;
    private:
        static shared_ptr<DataInteger> read(Reader &reader)
        {
            return make_shared<DataInteger>(reader.readS32());
        }
    public:
        virtual explicit operator wstring() const override
        {
            wostringstream os;
            os << value;
            return os.str();
        }
    };
    struct DataFloat final : public Data
    {
        virtual Type type() const override
        {
            return Type::Float;
        }
        float value;
        DataFloat(float value = 0)
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            return shared_ptr<Data>(new DataFloat(value));
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            writer.writeF32(value);
        }
        friend class Data;
    private:
        static shared_ptr<DataFloat> read(Reader &reader)
        {
            return make_shared<DataFloat>(reader.readFiniteF32());
        }
    public:
        virtual explicit operator wstring() const override
        {
            wostringstream os;
            os << value << L"f";
            return os.str();
        }
    };
    struct DataVector final : public Data
    {
        virtual Type type() const override
        {
            return Type::Vector;
        }
        VectorF value;
        DataVector(VectorF value = VectorF(0))
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            return shared_ptr<Data>(new DataVector(value));
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
        static shared_ptr<DataVector> read(Reader &reader)
        {
            VectorF value;
            value.x = reader.readFiniteF32();
            value.y = reader.readFiniteF32();
            value.z = reader.readFiniteF32();
            return make_shared<DataVector>(value);
        }
    public:
        virtual explicit operator wstring() const override
        {
            wostringstream os;
            os << L"<" << value.x << L", " << value.y << L", " << value.z << L">";
            return os.str();
        }
    };
    struct DataMatrix final : public Data
    {
        virtual Type type() const override
        {
            return Type::Matrix;
        }
        Matrix value;
        DataMatrix(Matrix value = Matrix::identity())
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            return shared_ptr<Data>(new DataMatrix(value));
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            for(int x = 0; x < 4; x++)
                for(int y = 0; y < 3; y++)
                {
                    writer.writeF32(value.get(x, y));
                }
        }
        friend class Data;
    private:
        static shared_ptr<DataMatrix> read(Reader &reader)
        {
            Matrix value;
            for(int x = 0; x < 4; x++)
                for(int y = 0; y < 3; y++)
                {
                    value.set(x, y, reader.readFiniteF32());
                }
            return make_shared<DataMatrix>(value);
        }
    public:
        virtual explicit operator wstring() const override
        {
            wostringstream os;
            for(int y = 0; y < 4; y++)
            {
                const wchar_t *str = L"|";
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
    struct DataList final : public Data
    {
        virtual Type type() const override
        {
            return Type::List;
        }
        vector<shared_ptr<Data>> value;
        DataList(const vector<shared_ptr<Data>> &value = vector<shared_ptr<Data>>())
            : value(value)
        {
        }
        DataList(vector<shared_ptr<Data>>  &&value)
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            auto retval = shared_ptr<DataList>(new DataList);
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
            assert((uint32_t)value.size() == value.size() && value.size() != (uint32_t) - 1);
            writer.writeU32((uint32_t)value.size());
            for(shared_ptr<Data> v : value)
            {
                assert(v);
                v->write(writer);
            }
        }
        friend class Data;
    private:
        static shared_ptr<DataList> read(Reader &reader)
        {
            shared_ptr<DataList> retval = make_shared<DataList>();
            size_t length = reader.readLimitedU32(0, (uint32_t) - 2);
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
            {
                return L"[]";
            }
            wostringstream os;
            const wchar_t * str = L"[";
            for(shared_ptr<Data> e : value)
            {
                os << str << (wstring)*e;
                str = L", ";
            }
            os << L"]";
            return os.str();
        }
    };
    struct DataObject final : public Data
    {
        virtual Type type() const override
        {
            return Type::Object;
        }
        unordered_map<wstring, shared_ptr<Data>> value;
        DataObject(const unordered_map<wstring, shared_ptr<Data>> &value = unordered_map<wstring, shared_ptr<Data>>())
            : value(value)
        {
        }
        DataObject(unordered_map<wstring, shared_ptr<Data>>  &&value)
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            auto retval = shared_ptr<DataObject>(new DataObject);
            for(pair<wstring, shared_ptr<Data>> e : value)
            {
                retval->value.insert(make_pair(get<0>(e), get<1>(e)->dup()));
            }
            return static_pointer_cast<Data>(retval);
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            assert((uint32_t)value.size() == value.size() && value.size() != (uint32_t) - 1);
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
        static shared_ptr<DataObject> read(Reader &reader)
        {
            shared_ptr<DataObject> retval = make_shared<DataObject>();
            size_t length = reader.readLimitedU32(0, (uint32_t) - 2);
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
            {
                return L"{}";
            }
            wostringstream os;
            const wchar_t * str = L"{";
            for(pair<wstring, shared_ptr<Data>> e : value)
            {
                os << str << L"\"" << get<0>(e) << L"\" = " << (wstring)*get<1>(e);
                str = L", ";
            }
            os << L"}";
            return os.str();
        }
    };
    struct DataString final : public Data
    {
        virtual Type type() const override
        {
            return Type::String;
        }
        wstring value;
        DataString(wstring value = L"")
            : value(value)
        {
        }
        virtual shared_ptr<Data> dup() const override
        {
            return shared_ptr<Data>(new DataString(value));
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            writer.writeString(value);
        }
        friend class Data;
    private:
        static shared_ptr<DataString> read(Reader &reader)
        {
            return make_shared<DataString>(reader.readString());
        }
    public:
        virtual explicit operator wstring() const override
        {
            return value;
        }
    };
    struct ScriptException final : public runtime_error
    {
        shared_ptr<Data> data;
        ScriptException(shared_ptr<Data> data)
            : runtime_error(wcsrtombs((wstring)*data)), data(data)
        {
        }
        ScriptException(wstring str)
            : ScriptException(static_pointer_cast<Data>(make_shared<DataString>(str)))
        {
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
            CastToObject,
            CastToBoolean,
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
            Neg,
            Abs,
            Sin,
            Cos,
            Tan,
            ATan,
            ASin,
            ACos,
            Exp,
            Log,
            Sqrt,
            ATan2,
            Conditional,
            MakeRotate,
            MakeRotateX,
            MakeRotateY,
            MakeRotateZ,
            MakeScale,
            MakeTranslate,
            Block,
            ListLiteral,
            NewObject,
            DoWhile,
            RemoveTranslate,
            Invert,
            Ceil,
            Floor,
            For,
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
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth = 0) const = 0;
        virtual void write(Writer &writer) const = 0;
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount);
    protected:
        static void checkStackDepth(unsigned stackDepth)
        {
            if(stackDepth > 1000)
            {
                throw ScriptException(L"stack depth limit exceeded");
            }
        }
    };
    struct State final
    {
        shared_ptr<DataObject> variables;
        unsigned loopCount = 0;
        void onLoop()
        {
            if(++loopCount >= 100000)
                throw ScriptException(L"too many loops");
        }
        const vector<shared_ptr<Node>> &nodes;
        State(const vector<shared_ptr<Node>> &nodes)
            : variables(make_shared<DataObject>()), nodes(nodes)
        {
        }
    };
    template<uint32_t size, typename ChildClass>
    struct NodeConstArgCount : public Node
    {
        friend class Node;
        uint32_t args[size];
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            shared_ptr<ChildClass> retval = make_shared<ChildClass>();
            for(uint32_t &v : retval->args)
            {
                v = reader.readLimitedU32(0, nodeCount - 1);
            }
            return static_pointer_cast<Node>(retval);
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
    struct NodeCast : public Node
    {
        uint32_t args[1];
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount, shared_ptr<NodeCast> retval)
        {
            for(uint32_t &v : retval->args)
            {
                v = reader.readLimitedU32(0, nodeCount - 1);
            }
            return static_pointer_cast<Node>(retval);
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
    struct NodeCastToString final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToString;
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            shared_ptr<Data> retval = state.nodes[args[0]]->evaluate(state, stackDepth + 1);
            return make_shared<DataString>((wstring) * retval);
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToString>());
        }
    };
    struct NodeCastToInteger final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToInteger;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Integer)
            {
                return retval;
            }
            if(retval->type() == Data::Type::Float)
            {
                return make_shared<DataInteger>(dynamic_pointer_cast<DataFloat>(retval)->value);
            }
            throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to integer");
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToInteger>());
        }
    };
    struct NodeCastToFloat final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToFloat;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
            {
                return retval;
            }
            if(retval->type() == Data::Type::Integer)
            {
                return make_shared<DataFloat>(dynamic_pointer_cast<DataInteger>(retval)->value);
            }
            throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to float");
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToFloat>());
        }
    };
    struct NodeCastToVector final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToVector;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Vector)
            {
                return retval;
            }
            if(retval->type() == Data::Type::Integer)
            {
                return make_shared<DataVector>(VectorF(dynamic_pointer_cast<DataInteger>(retval)->value));
            }
            if(retval->type() == Data::Type::Float)
            {
                return make_shared<DataVector>(VectorF(dynamic_pointer_cast<DataFloat>(retval)->value));
            }
            if(retval->type() == Data::Type::List)
            {
                DataList *list = dynamic_cast<DataList *>(retval.get());
                if(list->value.size() != 3)
                {
                    throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to vector");
                }
                VectorF v;
                v.x = dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[0]))->value;
                v.y = dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[1]))->value;
                v.z = dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[2]))->value;
                return shared_ptr<Data>(new DataVector(v));
            }
            throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to vector");
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToVector>());
        }
    };
    struct NodeCastToMatrix final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToMatrix;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Matrix)
            {
                return retval;
            }
            if(retval->type() == Data::Type::List)
            {
                DataList *list = dynamic_cast<DataList *>(retval.get());
                if(list->value.size() != 16)
                {
                    throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to matrix");
                }
                Matrix v;
                for(int i = 0, y = 0; y < 4; y++)
                {
                    for(int x = 0; x < 4; x++, i++)
                    {
                        v.set(x, y, dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[i]))->value);
                    }
                }
                return shared_ptr<Data>(new DataMatrix(v));
            }
            throw ScriptException(make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to matrix"));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToMatrix>());
        }
    };
    struct NodeCastToList final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToList;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::List)
            {
                return retval;
            }
            if(retval->type() == Data::Type::Vector)
            {
                VectorF value = dynamic_cast<DataVector *>(retval.get())->value;
                shared_ptr<DataList> retval2 = make_shared<DataList>();
                retval2->value.push_back(make_shared<DataFloat>(value.x));
                retval2->value.push_back(make_shared<DataFloat>(value.y));
                retval2->value.push_back(make_shared<DataFloat>(value.z));
                return static_pointer_cast<Data>(retval2);
            }
            if(retval->type() == Data::Type::Matrix)
            {
                Matrix value = dynamic_cast<DataMatrix *>(retval.get())->value;
                shared_ptr<DataList> retval2 = make_shared<DataList>();
                for(int y = 0; y < 4; y++)
                {
                    for(int x = 0; x < 4; x++)
                    {
                        retval2->value.push_back(make_shared<DataFloat>(value.get(x, y)));
                    }
                }
                return static_pointer_cast<Data>(retval2);
            }
            if(retval->type() == Data::Type::String)
            {
                wstring value = dynamic_pointer_cast<DataString>(retval)->value;
                shared_ptr<DataList> retval2 = make_shared<DataList>();
                retval2->value.reserve(value.size());
                for(size_t i = 0; i < value.size(); i++)
                    retval2->value.push_back(make_shared<DataString>(wstring(L"") + value[i]));
                return retval2;
            }
            throw ScriptException(make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to list"));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToList>());
        }
    };
    struct NodeCastToObject final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToObject;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Object)
            {
                return retval;
            }
            throw ScriptException(make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to object"));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToObject>());
        }
    };
    struct NodeCastToBoolean final : public NodeCast
    {
        virtual Type type() const override
        {
            return Type::CastToBoolean;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Boolean)
            {
                return retval;
            }
            throw ScriptException(make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to boolean"));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
        friend class Node;
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            return NodeCast::read(reader, nodeCount, make_shared<NodeCastToBoolean>());
        }
    };
    struct NodeReadIndex final : public NodeConstArgCount<2, NodeReadIndex>
    {
        virtual Type type() const override
        {
            return Type::ReadIndex;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg2->type() == Data::Type::String)
            {
                wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                if(value == L"dup")
                {
                    return arg1->dup();
                }
            }
            if(arg1->type() == Data::Type::Vector)
            {
                if(arg2->type() == Data::Type::Integer)
                {
                    switch(dynamic_cast<DataInteger *>(arg2.get())->value)
                    {
                    case 0:
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x));
                    case 1:
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y));
                    case 2:
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z));
                    default:
                        throw ScriptException(L"index out of range");
                    }
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    if(value == L"x")
                    {
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x));
                    }
                    if(value == L"y")
                    {
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y));
                    }
                    if(value == L"z")
                    {
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z));
                    }
                    if(value == L"length")
                    {
                        return shared_ptr<Data>(new DataInteger(3));
                    }
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::Matrix)
            {
                if(arg2->type() == Data::Type::Integer)
                {
                    int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                    if(value < 0 || value >= 16)
                    {
                        throw ScriptException(L"index out of range");
                    }
                    return shared_ptr<Data>(new DataFloat(dynamic_cast<DataMatrix *>(arg1.get())->value.get(value % 4, value / 4)));
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    if(value == L"length")
                    {
                        return shared_ptr<Data>(new DataInteger(16));
                    }
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::List)
            {
                const vector<shared_ptr<Data>> &list = dynamic_cast<DataList *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Integer)
                {
                    int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                    if(value < 0 || (size_t)value >= list.size())
                    {
                        throw ScriptException(L"index out of range");
                    }
                    return list[value];
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    if(value == L"length")
                    {
                        return shared_ptr<Data>(new DataInteger(list.size()));
                    }
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::Object)
            {
                const unordered_map<wstring, shared_ptr<Data>> &map = dynamic_cast<DataObject *>(arg1.get())->value;
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    auto i = map.find(value);
                    if(i == map.end())
                    {
                        throw ScriptException(L"variable doesn't exist : " + value);
                    }
                    return get<1>(*i);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::String)
            {
                wstring str = dynamic_cast<DataString *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Integer)
                {
                    int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                    if(value < 0 || (size_t)value >= str.size())
                    {
                        throw ScriptException(L"index out of range");
                    }
                    return shared_ptr<Data>(new DataString(str.substr(value, 1)));
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    if(value == L"length")
                    {
                        return shared_ptr<Data>(new DataInteger(str.size()));
                    }
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            throw ScriptException(L"invalid type to index");
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeAssignIndex final : public NodeConstArgCount<3, NodeAssignIndex>
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
                    int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                    if(value < 0 || value >= 3)
                    {
                        throw ScriptException(L"index out of range");
                    }
                    if(arg3->type() != Data::Type::Float)
                    {
                        throw ScriptException(L"can't assign " + arg3->typeString() + L" to float");
                    }
                    float newValue = dynamic_cast<DataFloat *>(arg3.get())->value;
                    switch(value)
                    {
                    case 0:
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x = newValue));
                    case 1:
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y = newValue));
                    case 2:
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z = newValue));
                    default:
                        assert(false);
                    }
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    if(value != L"x" && value != L"y" && value != L"z")
                    {
                        throw ScriptException(L"can't write to " + value);
                    }
                    if(arg3->type() != Data::Type::Float)
                    {
                        throw ScriptException(L"can't assign " + arg3->typeString() + L" to a float");
                    }
                    float newValue = dynamic_cast<DataFloat *>(arg3.get())->value;
                    if(value == L"x")
                    {
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x = newValue));
                    }
                    if(value == L"y")
                    {
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y = newValue));
                    }
                    if(value == L"z")
                    {
                        return shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z = newValue));
                    }
                    if(value == L"length")
                    {
                        return shared_ptr<Data>(new DataInteger(3));
                    }
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::Matrix)
            {
                if(arg2->type() == Data::Type::Integer)
                {
                    int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                    if(arg3->type() != Data::Type::Float)
                    {
                        throw ScriptException(L"can't assign " + arg3->typeString() + L" to a float");
                    }
                    float newValue = dynamic_cast<DataFloat *>(arg3.get())->value;
                    if(value < 0 || value >= 16)
                    {
                        throw ScriptException(L"index out of range");
                    }
                    dynamic_cast<DataMatrix *>(arg1.get())->value.set(value % 4, value / 4, newValue);
                    return shared_ptr<Data>(new DataFloat(newValue));
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    throw ScriptException(L"can't write to " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::List)
            {
                vector<shared_ptr<Data>> &list = dynamic_cast<DataList *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Integer)
                {
                    int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                    if(value < 0 || (size_t)value >= list.size())
                    {
                        throw ScriptException(L"index out of range");
                    }
                    return list[value] = arg3->dup();
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    throw ScriptException(L"can't write to " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::Object)
            {
                unordered_map<wstring, shared_ptr<Data>> &map = dynamic_cast<DataObject *>(arg1.get())->value;
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    return map[value] = arg3->dup();
                }
                throw ScriptException(L"illegal type for index");
            }
            if(arg1->type() == Data::Type::String)
            {
                wstring & str = dynamic_cast<DataString *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Integer)
                {
                    int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                    if(value < 0 || (size_t)value >= str.size())
                    {
                        throw ScriptException(L"index out of range");
                    }
                    str = str.substr(0, value) + (wstring)*arg3 + str.substr(value + 1);
                }
                if(arg2->type() == Data::Type::String)
                {
                    wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                    throw ScriptException(L"can't write to " + value);
                }
                throw ScriptException(L"illegal type for index");
            }
            throw ScriptException(L"invalid type to index");
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1), state.nodes[args[2]]->evaluate(state, stackDepth + 1));
        }
    };
    template <typename ChildClass>
    struct NodeBinaryArithmatic : public NodeConstArgCount<2, ChildClass>
    {
        static shared_ptr<Data> toData(float v)
        {
            return shared_ptr<Data>(new DataFloat(v));
        }
        static shared_ptr<Data> toData(int32_t v)
        {
            return shared_ptr<Data>(new DataInteger(v));
        }
        static shared_ptr<Data> toData(VectorF v)
        {
            return shared_ptr<Data>(new DataVector(v));
        }
        static shared_ptr<Data> toData(Matrix v)
        {
            return shared_ptr<Data>(new DataMatrix(v));
        }
        static shared_ptr<Data> toData(bool v)
        {
            return shared_ptr<Data>(new DataBoolean(v));
        }
        static int32_t throwError()
        {
            throw ScriptException(L"invalid types for " + ChildClass::operatorString());
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data>, shared_ptr<Data>)
        {
            throwError();
            return nullptr;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::Float)
            {
                auto v1 = dynamic_cast<DataFloat *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Integer)
            {
                auto v1 = dynamic_cast<DataInteger *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Vector)
            {
                auto v1 = dynamic_cast<DataVector *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Matrix)
            {
                auto v1 = dynamic_cast<DataMatrix *>(arg1.get())->value;
                if(arg2->type() == Data::Type::Float)
                {
                    auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Integer)
                {
                    auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Vector)
                {
                    auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
                if(arg2->type() == Data::Type::Matrix)
                {
                    auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                    return toData(ChildClass::evalFn(v1, v2));
                }
            }
            if(arg1->type() == Data::Type::Boolean && arg2->type() == Data::Type::Boolean)
            {
                auto v1 = dynamic_cast<DataBoolean *>(arg1.get())->value;
                auto v2 = dynamic_cast<DataBoolean *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            return ChildClass::evaluateBackup(arg1, arg2);
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            Node::checkStackDepth(stackDepth);
            return evaluate(state.nodes[NodeConstArgCount<2, ChildClass>::args[0]]->evaluate(state, stackDepth + 1), state.nodes[NodeConstArgCount<2, ChildClass>::args[1]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeAdd : public NodeBinaryArithmatic<NodeAdd>
    {
        static wstring operatorString()
        {
            return L"+";
        }
        virtual Type type() const override
        {
            return Type::Add;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static VectorF evalFn(VectorF a, VectorF b)
        {
            return a + b;
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static float evalFn(float a, float b)
        {
            return a + b;
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            return a + b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeSub : public NodeBinaryArithmatic<NodeSub>
    {
        static wstring operatorString()
        {
            return L"-";
        }
        virtual Type type() const override
        {
            return Type::Sub;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static VectorF evalFn(VectorF a, VectorF b)
        {
            return a - b;
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static float evalFn(float a, float b)
        {
            return a - b;
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            return a - b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeMul : public NodeBinaryArithmatic<NodeMul>
    {
        static wstring operatorString()
        {
            return L"*";
        }
        virtual Type type() const override
        {
            return Type::Mul;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static VectorF evalFn(VectorF a, VectorF b)
        {
            return a - b;
        }
        static VectorF evalFn(float a, VectorF b)
        {
            return a * b;
        }
        static VectorF evalFn(VectorF a, float b)
        {
            return a * b;
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static float evalFn(float a, float b)
        {
            return a * b;
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            return a * b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeDiv : public NodeBinaryArithmatic<NodeDiv>
    {
        static wstring operatorString()
        {
            return L"/";
        }
        virtual Type type() const override
        {
            return Type::Div;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static VectorF evalFn(VectorF a, VectorF b)
        {
            if(b.x == 0 || b.y == 0 || b.z == 0)
            {
                throw ScriptException(L"divide by zero");
            }
            return a / b;
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static VectorF evalFn(VectorF a, float b)
        {
            if(b == 0)
            {
                throw ScriptException(L"divide by zero");
            }
            return a / b;
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static float evalFn(float a, float b)
        {
            if(b == 0)
            {
                throw ScriptException(L"divide by zero");
            }
            return a / b;
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            if(b == 0)
            {
                throw ScriptException(L"divide by zero");
            }
            return a / b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeMod : public NodeBinaryArithmatic<NodeMod>
    {
        static wstring operatorString()
        {
            return L"%";
        }
        virtual Type type() const override
        {
            return Type::Mod;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static float evalFn(float a, float b)
        {
            if(b == 0)
            {
                throw ScriptException(L"divide by zero");
            }
            return fmod(a, b);
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            if(b == 0)
            {
                throw ScriptException(L"divide by zero");
            }
            return a % b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodePow : public NodeBinaryArithmatic<NodePow>
    {
        static wstring operatorString()
        {
            return L"**";
        }
        virtual Type type() const override
        {
            return Type::Pow;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static float evalFn(float a, float b)
        {
            if(b != (int)b && a <= 0)
            {
                throw ScriptException(L"domain error");
            }
            if(b <= 0 && a <= 0)
            {
                throw ScriptException(L"domain error");
            }
            return pow(a, b);
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeAnd : public NodeBinaryArithmatic<NodeAnd>
    {
        static wstring operatorString()
        {
            return L"and";
        }
        virtual Type type() const override
        {
            return Type::And;
        }
        static bool evalFn(bool a, bool b)
        {
            return a && b;
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            return a & b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeOr : public NodeBinaryArithmatic<NodeOr>
    {
        static wstring operatorString()
        {
            return L"or";
        }
        virtual Type type() const override
        {
            return Type::Or;
        }
        static bool evalFn(bool a, bool b)
        {
            return a || b;
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            return a | b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeXor : public NodeBinaryArithmatic<NodeXor>
    {
        static wstring operatorString()
        {
            return L"xor";
        }
        virtual Type type() const override
        {
            return Type::Xor;
        }
        static bool evalFn(bool a, bool b)
        {
            return a != b;
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(int32_t a, int32_t b)
        {
            return a ^ b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeConcat : public NodeBinaryArithmatic<NodeConcat>
    {
        static wstring operatorString()
        {
            return L"~";
        }
        virtual Type type() const override
        {
            return Type::Concat;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static Matrix evalFn(Matrix a, Matrix b)
        {
            return a.concat(b);
        }
        static VectorF evalFn(Matrix a, VectorF b)
        {
            return a.apply(b);
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(int32_t, int32_t) // error case
        {
            return throwError();
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::String || arg2->type() == Data::Type::String)
            {
                return shared_ptr<Data>(new DataString((wstring)*arg1 + (wstring)*arg2));
            }
            if(arg1->type() == Data::Type::List || arg2->type() == Data::Type::List)
            {
                shared_ptr<DataList> list = make_shared<DataList>();
                for(shared_ptr<Data> v : dynamic_cast<DataList *>(arg1.get())->value)
                {
                    list->value.push_back(v->dup());
                }
                for(shared_ptr<Data> v : dynamic_cast<DataList *>(arg2.get())->value)
                {
                    list->value.push_back(v->dup());
                }
                return static_pointer_cast<Data>(list);
            }
            throwError();
            return nullptr;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeDot : public NodeBinaryArithmatic<NodeDot>
    {
        static wstring operatorString()
        {
            return L"dot";
        }
        virtual Type type() const override
        {
            return Type::Dot;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static float evalFn(VectorF a, VectorF b)
        {
            return dot(a, b);
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(int32_t, int32_t) // error case
        {
            return throwError();
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeCross : public NodeBinaryArithmatic<NodeCross>
    {
        static wstring operatorString()
        {
            return L"cross";
        }
        virtual Type type() const override
        {
            return Type::Cross;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static VectorF evalFn(VectorF a, VectorF b)
        {
            return cross(a, b);
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(int32_t, int32_t) // error case
        {
            return throwError();
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
    };
    struct NodeEqual : public NodeBinaryArithmatic<NodeEqual>
    {
        static wstring operatorString()
        {
            return L"==";
        }
        virtual Type type() const override
        {
            return Type::Equal;
        }
        static bool evalFn(bool a, bool b)
        {
            return a == b;
        }
        static bool evalFn(VectorF a, VectorF b)
        {
            return a == b;
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static bool evalFn(Matrix a, Matrix b)
        {
            return a == b;
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static bool evalFn(float a, float b)
        {
            return a == b;
        }
        static bool evalFn(int32_t a, int32_t b)
        {
            return a == b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
            {
                return shared_ptr<Data>(new DataBoolean((wstring)*arg1 == (wstring)*arg2));
            }
            if(arg1->type() == Data::Type::List && arg2->type() == Data::Type::List)
            {
                const vector<shared_ptr<Data>> &a = dynamic_cast<DataList *>(arg1.get())->value, &b = dynamic_cast<DataList *>(arg2.get())->value;
                if(a.size() != b.size())
                {
                    return shared_ptr<Data>(new DataBoolean(false));
                }
                for(size_t i = 0; i < a.size(); i++)
                {
                    if(a[i]->type() != b[i]->type())
                    {
                        return shared_ptr<Data>(new DataBoolean(false));
                    }
                    if(!dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(a[i], b[i]))->value)
                    {
                        return shared_ptr<Data>(new DataBoolean(false));
                    }
                }
                return shared_ptr<Data>(new DataBoolean(true));
            }
            if(arg1->type() == Data::Type::Object && arg2->type() == Data::Type::Object)
            {
                const unordered_map<wstring, shared_ptr<Data>> &a = dynamic_cast<DataObject *>(arg1.get())->value, &b = dynamic_cast<DataObject *>(arg2.get())->value;
                if(a.size() != b.size())
                {
                    return shared_ptr<Data>(new DataBoolean(false));
                }
                for(pair<wstring, shared_ptr<Data>> v : a)
                {
                    shared_ptr<Data> va = get<1>(v);
                    auto iter = b.find(get<0>(v));
                    if(iter == b.end())
                    {
                        return shared_ptr<Data>(new DataBoolean(false));
                    }
                    shared_ptr<Data> vb = get<1>(*iter);
                    if(va->type() != vb->type())
                    {
                        return shared_ptr<Data>(new DataBoolean(false));
                    }
                    if(!dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(va, vb))->value)
                    {
                        return shared_ptr<Data>(new DataBoolean(false));
                    }
                }
                return shared_ptr<Data>(new DataBoolean(true));
            }
            throwError();
            return nullptr;
        }
    };
    struct NodeNotEqual : public NodeBinaryArithmatic<NodeNotEqual>
    {
        static wstring operatorString()
        {
            return L"!=";
        }
        virtual Type type() const override
        {
            return Type::NotEqual;
        }
        static bool evalFn(bool a, bool b)
        {
            return a != b;
        }
        static bool evalFn(VectorF a, VectorF b)
        {
            return a != b;
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static bool evalFn(Matrix a, Matrix b)
        {
            return a != b;
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static bool evalFn(float a, float b)
        {
            return a != b;
        }
        static bool evalFn(int32_t a, int32_t b)
        {
            return a != b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
            {
                return shared_ptr<Data>(new DataBoolean((wstring)*arg1 != (wstring)*arg2));
            }
            if(arg1->type() == Data::Type::List && arg2->type() == Data::Type::List)
            {
                const vector<shared_ptr<Data>> &a = dynamic_cast<DataList *>(arg1.get())->value, &b = dynamic_cast<DataList *>(arg2.get())->value;
                if(a.size() != b.size())
                {
                    return shared_ptr<Data>(new DataBoolean(true));
                }
                for(size_t i = 0; i < a.size(); i++)
                {
                    if(a[i]->type() != b[i]->type())
                    {
                        return shared_ptr<Data>(new DataBoolean(true));
                    }
                    if(!dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(a[i], b[i]))->value)
                    {
                        return shared_ptr<Data>(new DataBoolean(true));
                    }
                }
                return shared_ptr<Data>(new DataBoolean(false));
            }
            if(arg1->type() == Data::Type::Object && arg2->type() == Data::Type::Object)
            {
                const unordered_map<wstring, shared_ptr<Data>> &a = dynamic_cast<DataObject *>(arg1.get())->value, &b = dynamic_cast<DataObject *>(arg2.get())->value;
                if(a.size() != b.size())
                {
                    return shared_ptr<Data>(new DataBoolean(true));
                }
                for(pair<wstring, shared_ptr<Data>> v : a)
                {
                    shared_ptr<Data> va = get<1>(v);
                    auto iter = b.find(get<0>(v));
                    if(iter == b.end())
                    {
                        return shared_ptr<Data>(new DataBoolean(true));
                    }
                    shared_ptr<Data> vb = get<1>(*iter);
                    if(va->type() != vb->type())
                    {
                        return shared_ptr<Data>(new DataBoolean(true));
                    }
                    if(!dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(va, vb))->value)
                    {
                        return shared_ptr<Data>(new DataBoolean(true));
                    }
                }
                return shared_ptr<Data>(new DataBoolean(false));
            }
            throwError();
            return nullptr;
        }
    };
    struct NodeLessThan : public NodeBinaryArithmatic<NodeLessThan>
    {
        static wstring operatorString()
        {
            return L"<";
        }
        virtual Type type() const override
        {
            return Type::LessThan;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static bool evalFn(float a, float b)
        {
            return a < b;
        }
        static bool evalFn(int32_t a, int32_t b)
        {
            return a < b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
            {
                return shared_ptr<Data>(new DataBoolean((wstring)*arg1 < (wstring)*arg2));
            }
            throwError();
            return nullptr;
        }
    };
    struct NodeGreaterThan : public NodeBinaryArithmatic<NodeGreaterThan>
    {
        static wstring operatorString()
        {
            return L">";
        }
        virtual Type type() const override
        {
            return Type::GreaterThan;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static bool evalFn(float a, float b)
        {
            return a > b;
        }
        static bool evalFn(int32_t a, int32_t b)
        {
            return a > b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
            {
                return shared_ptr<Data>(new DataBoolean((wstring)*arg1 > (wstring)*arg2));
            }
            throwError();
            return nullptr;
        }
    };
    struct NodeLessEqual : public NodeBinaryArithmatic<NodeLessEqual>
    {
        static wstring operatorString()
        {
            return L"<=";
        }
        virtual Type type() const override
        {
            return Type::LessEqual;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static bool evalFn(float a, float b)
        {
            return a <= b;
        }
        static bool evalFn(int32_t a, int32_t b)
        {
            return a <= b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
            {
                return shared_ptr<Data>(new DataBoolean((wstring)*arg1 <= (wstring)*arg2));
            }
            throwError();
            return nullptr;
        }
    };
    struct NodeGreaterEqual : public NodeBinaryArithmatic<NodeGreaterEqual>
    {
        static wstring operatorString()
        {
            return L">=";
        }
        virtual Type type() const override
        {
            return Type::GreaterEqual;
        }
        static int32_t evalFn(bool, bool) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, VectorF) // error case
        {
            return throwError();
        }
        static int32_t evalFn(Matrix, float) // error case
        {
            return throwError();
        }
        static int32_t evalFn(float, Matrix) // error case
        {
            return throwError();
        }
        static int32_t evalFn(VectorF, Matrix) // error case
        {
            return throwError();
        }
        static bool evalFn(float a, float b)
        {
            return a >= b;
        }
        static bool evalFn(int32_t a, int32_t b)
        {
            return a >= b;
        }
        static auto evalFn(float a, int32_t b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static auto evalFn(int32_t a, float b) -> decltype(evalFn((float)a, (float)b))
        {
            return evalFn((float)a, (float)b);
        }
        static shared_ptr<Data> evaluateBackup(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
            {
                return shared_ptr<Data>(new DataBoolean((wstring)*arg1 >= (wstring)*arg2));
            }
            throwError();
            return nullptr;
        }
    };
    struct NodeNeg final : public NodeConstArgCount<1, NodeNeg>
    {
        virtual Type type() const override
        {
            return Type::Neg;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Vector)
            {
                return shared_ptr<Data>(new DataVector(-dynamic_cast<DataVector *>(retval.get())->value));
            }
            if(retval->type() == Data::Type::Float)
            {
                return shared_ptr<Data>(new DataFloat(-dynamic_cast<DataFloat *>(retval.get())->value));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataInteger(-dynamic_cast<DataInteger *>(retval.get())->value));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for - : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeNot final : public NodeConstArgCount<1, NodeNot>
    {
        virtual Type type() const override
        {
            return Type::Not;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Boolean)
            {
                return shared_ptr<Data>(new DataBoolean(!dynamic_cast<DataBoolean *>(retval.get())->value));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataInteger(~dynamic_cast<DataInteger *>(retval.get())->value));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for not : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeAbs final : public NodeConstArgCount<1, NodeAbs>
    {
        virtual Type type() const override
        {
            return Type::Abs;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Vector)
            {
                return shared_ptr<Data>(new DataFloat(abs(dynamic_cast<DataVector *>(retval.get())->value)));
            }
            if(retval->type() == Data::Type::Float)
            {
                return shared_ptr<Data>(new DataFloat(abs(dynamic_cast<DataFloat *>(retval.get())->value)));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataInteger(abs(dynamic_cast<DataInteger *>(retval.get())->value)));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for abs : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeSin final : public NodeConstArgCount<1, NodeSin>
    {
        virtual Type type() const override
        {
            return Type::Sin;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
            {
                return shared_ptr<Data>(new DataFloat(sin(dynamic_cast<DataFloat *>(retval.get())->value)));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataFloat(sin(dynamic_cast<DataInteger *>(retval.get())->value)));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for sin : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeCos final : public NodeConstArgCount<1, NodeCos>
    {
        virtual Type type() const override
        {
            return Type::Cos;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
            {
                return shared_ptr<Data>(new DataFloat(cos(dynamic_cast<DataFloat *>(retval.get())->value)));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataFloat(cos(dynamic_cast<DataInteger *>(retval.get())->value)));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for cos : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeTan final : public NodeConstArgCount<1, NodeTan>
    {
        virtual Type type() const override
        {
            return Type::Tan;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
            {
                float value = dynamic_cast<DataFloat *>(retval.get())->value;
                if(abs(cos(value)) < 1e-6)
                {
                    throw ScriptException(L"tan domain error");
                }
                return shared_ptr<Data>(new DataFloat(tan(value)));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataFloat(tan(dynamic_cast<DataInteger *>(retval.get())->value)));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for tan : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeATan final : public NodeConstArgCount<1, NodeATan>
    {
        virtual Type type() const override
        {
            return Type::ATan;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
            {
                float value = dynamic_cast<DataFloat *>(retval.get())->value;
                return shared_ptr<Data>(new DataFloat(atan(value)));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataFloat(atan(dynamic_cast<DataInteger *>(retval.get())->value)));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for atan : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeASin final : public NodeConstArgCount<1, NodeASin>
    {
        virtual Type type() const override
        {
            return Type::ASin;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for asin : " + retval->typeString()));
            }
            if(value < -1 || value > 1)
            {
                throw ScriptException(L"asin domain error");
            }
            return shared_ptr<Data>(new DataFloat(asin(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeACos final : public NodeConstArgCount<1, NodeACos>
    {
        virtual Type type() const override
        {
            return Type::ACos;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for acos : " + retval->typeString()));
            }
            if(value < -1 || value > 1)
            {
                throw ScriptException(L"acos domain error");
            }
            return shared_ptr<Data>(new DataFloat(acos(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeExp final : public NodeConstArgCount<1, NodeExp>
    {
        virtual Type type() const override
        {
            return Type::Exp;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for exp : " + retval->typeString()));
            }
            if(value > 88.7228)
            {
                throw ScriptException(L"exp overflow error");
            }
            return shared_ptr<Data>(new DataFloat(exp(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeLog final : public NodeConstArgCount<1, NodeLog>
    {
        virtual Type type() const override
        {
            return Type::Log;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for log : " + retval->typeString()));
            }
            if(value <= 0)
            {
                throw ScriptException(L"log domain error");
            }
            return shared_ptr<Data>(new DataFloat(log(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeSqrt final : public NodeConstArgCount<1, NodeSqrt>
    {
        virtual Type type() const override
        {
            return Type::Sqrt;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for sqrt : " + retval->typeString()));
            }
            if(value < 0)
            {
                throw ScriptException(L"sqrt domain error");
            }
            return shared_ptr<Data>(new DataFloat(sqrt(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeATan2 final : public NodeConstArgCount<2, NodeATan2>
    {
        virtual Type type() const override
        {
            return Type::ATan2;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            float v1;
            if(arg1->type() == Data::Type::Float)
            {
                v1 = dynamic_cast<DataFloat *>(arg1.get())->value;
            }
            else if(arg1->type() == Data::Type::Integer)
            {
                v1 = dynamic_cast<DataInteger *>(arg1.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for atan2 : " + arg1->typeString()));
            }
            float v2;
            if(arg2->type() == Data::Type::Float)
            {
                v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
            }
            else if(arg2->type() == Data::Type::Integer)
            {
                v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for atan2 : " + arg2->typeString()));
            }
            if(v1 == 0 && v2 == 0)
            {
                throw ScriptException(L"sqrt domain error");
            }
            return shared_ptr<Data>(new DataFloat(atan2(v1, v2)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeConditional final : public NodeConstArgCount<3, NodeConditional>
    {
        virtual Type type() const override
        {
            return Type::Conditional;
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            shared_ptr<Data> condition = state.nodes[args[0]]->evaluate(state, stackDepth + 1);
            if(condition->type() != Data::Type::Boolean)
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for conditional : " + condition->typeString()));
            }
            if(dynamic_cast<DataBoolean *>(condition.get())->value)
            {
                return state.nodes[args[1]]->evaluate(state, stackDepth + 1);
            }
            return state.nodes[args[2]]->evaluate(state, stackDepth + 1);
        }
    };
    struct NodeMakeRotate final : public NodeConstArgCount<2, NodeMakeRotate>
    {
        virtual Type type() const override
        {
            return Type::MakeRotate;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> arg1, shared_ptr<Data> arg2)
        {
            VectorF v1;
            if(arg1->type() == Data::Type::Vector)
            {
                v1 = dynamic_cast<DataVector *>(arg1.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for make_rotate : " + arg1->typeString()));
            }
            float v2;
            if(arg2->type() == Data::Type::Float)
            {
                v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
            }
            else if(arg2->type() == Data::Type::Integer)
            {
                v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for make_rotate : " + arg2->typeString()));
            }
            if(v1 == VectorF(0))
            {
                throw ScriptException(L"make_rotate called with <0, 0, 0>");
            }
            return shared_ptr<Data>(new DataMatrix(Matrix::rotate(v1, v2)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeMakeRotateX final : public NodeConstArgCount<1, NodeMakeRotateX>
    {
        virtual Type type() const override
        {
            return Type::MakeRotateX;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for make_rotatex : " + retval->typeString()));
            }
            return shared_ptr<Data>(new DataMatrix(Matrix::rotateX(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeMakeRotateY final : public NodeConstArgCount<1, NodeMakeRotateY>
    {
        virtual Type type() const override
        {
            return Type::MakeRotateY;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for make_rotatey : " + retval->typeString()));
            }
            return shared_ptr<Data>(new DataMatrix(Matrix::rotateY(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeMakeRotateZ final : public NodeConstArgCount<1, NodeMakeRotateZ>
    {
        virtual Type type() const override
        {
            return Type::MakeRotateZ;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            float value;
            if(retval->type() == Data::Type::Float)
            {
                value = dynamic_cast<DataFloat *>(retval.get())->value;
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = dynamic_cast<DataInteger *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for make_rotatez : " + retval->typeString()));
            }
            return shared_ptr<Data>(new DataMatrix(Matrix::rotateZ(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeMakeScale final : public NodeConstArgCount<1, NodeMakeScale>
    {
        virtual Type type() const override
        {
            return Type::MakeScale;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            VectorF value;
            if(retval->type() == Data::Type::Float)
            {
                value = VectorF(dynamic_cast<DataFloat *>(retval.get())->value);
            }
            else if(retval->type() == Data::Type::Integer)
            {
                value = VectorF(dynamic_cast<DataInteger *>(retval.get())->value);
            }
            else if(retval->type() == Data::Type::Vector)
            {
                value = dynamic_cast<DataVector *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for make_scale : " + retval->typeString()));
            }
            return shared_ptr<Data>(new DataMatrix(Matrix::scale(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeMakeTranslate final : public NodeConstArgCount<1, NodeMakeTranslate>
    {
        virtual Type type() const override
        {
            return Type::MakeScale;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            VectorF value;
            if(retval->type() == Data::Type::Vector)
            {
                value = dynamic_cast<DataVector *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for make_translate : " + retval->typeString()));
            }
            return shared_ptr<Data>(new DataMatrix(Matrix::translate(value)));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeBlock final : public Node
    {
        friend class Node;
        vector<uint32_t> nodes;
        virtual Type type() const override
        {
            return Type::Block;
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            shared_ptr<Data> retval = shared_ptr<Data>(new DataInteger(0));
            for(uint32_t n : nodes)
            {
                retval = state.nodes[n]->evaluate(state, stackDepth + 1);
            }
            return retval;
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            assert((uint32_t)nodes.size() == nodes.size() && nodes.size() != (uint32_t) - 1);
            writer.writeU32((uint32_t)nodes.size());
            for(uint32_t v : nodes)
            {
                writer.writeU32(v);
            }
        }
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            shared_ptr<NodeBlock> retval = make_shared<NodeBlock>();
            size_t length = reader.readLimitedU32(0, (uint32_t) - 2);
            retval->nodes.reserve(length);
            for(size_t i = 0; i < length; i++)
            {
                retval->nodes.push_back(reader.readLimitedU32(0, nodeCount - 1));
            }
            return static_pointer_cast<Node>(retval);
        }
    };
    struct NodeListLiteral final : public Node
    {
        friend class Node;
        vector<uint32_t> nodes;
        virtual Type type() const override
        {
            return Type::ListLiteral;
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            shared_ptr<DataList> retval = make_shared<DataList>();
            for(uint32_t n : nodes)
            {
                retval->value.push_back(state.nodes[n]->evaluate(state, stackDepth + 1));
            }
            return static_pointer_cast<Data>(retval);
        }
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
            assert((uint32_t)nodes.size() == nodes.size() && nodes.size() != (uint32_t) - 1);
            writer.writeU32((uint32_t)nodes.size());
            for(uint32_t v : nodes)
            {
                writer.writeU32(v);
            }
        }
    protected:
        static shared_ptr<Node> read(Reader &reader, uint32_t nodeCount)
        {
            shared_ptr<NodeBlock> retval = make_shared<NodeBlock>();
            size_t length = reader.readLimitedU32(0, (uint32_t) - 2);
            retval->nodes.reserve(length);
            for(size_t i = 0; i < length; i++)
            {
                retval->nodes.push_back(reader.readLimitedU32(0, nodeCount - 1));
            }
            return static_pointer_cast<Node>(retval);
        }
    };
    struct NodeNewObject final : public Node
    {
        friend class Node;
        virtual Type type() const override
        {
            return Type::NewObject;
        }
    protected:
        static shared_ptr<Node> read(Reader &, uint32_t)
        {
            return make_shared<NodeNewObject>();
        }
    public:
        virtual void write(Writer &writer) const override
        {
            writeType(writer, type());
        }
        virtual shared_ptr<Data> evaluate(State &, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return shared_ptr<Data>(new DataObject);
        }
    };
    struct NodeDoWhile final : public NodeConstArgCount<2, NodeDoWhile>
    {
        virtual Type type() const override
        {
            return Type::DoWhile;
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            shared_ptr<Data> retval;
            while(true)
            {
                state.onLoop();
                retval = state.nodes[args[0]]->evaluate(state, stackDepth + 1);
                shared_ptr<Data> condition = state.nodes[args[1]]->evaluate(state, stackDepth + 1);
                if(condition->type() != Data::Type::Boolean)
                {
                    throw ScriptException(make_shared<DataString>(L"invalid type for conditional : " + condition->typeString()));
                }
                if(!dynamic_cast<DataBoolean *>(condition.get())->value)
                {
                    return retval;
                }
            }
        }
    };
    struct NodeRemoveTranslate final : public NodeConstArgCount<1, NodeRemoveTranslate>
    {
        virtual Type type() const override
        {
            return Type::RemoveTranslate;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            Matrix value;
            if(retval->type() == Data::Type::Matrix)
            {
                value = dynamic_cast<DataMatrix *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for remove_translate : " + retval->typeString()));
            }
            value.set(3,0,0);
            value.set(3,1,0);
            value.set(3,2,0);
            return shared_ptr<Data>(new DataMatrix(value));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeInvert final : public NodeConstArgCount<1, NodeInvert>
    {
        virtual Type type() const override
        {
            return Type::Invert;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            Matrix value;
            if(retval->type() == Data::Type::Matrix)
            {
                value = dynamic_cast<DataMatrix *>(retval.get())->value;
            }
            else
            {
                throw ScriptException(make_shared<DataString>(L"invalid type for invert : " + retval->typeString()));
            }
            try
            {
                value = value.invert();
            }
            catch(domain_error &)
            {
                throw ScriptException(L"can't invert singular matrix");
            }
            return shared_ptr<Data>(new DataMatrix(value));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeCeil final : public NodeConstArgCount<1, NodeCeil>
    {
        virtual Type type() const override
        {
            return Type::Ceil;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
            {
                return shared_ptr<Data>(new DataInteger((int32_t)ceil(dynamic_cast<DataFloat *>(retval.get())->value)));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataInteger(dynamic_cast<DataInteger *>(retval.get())->value));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for ceil : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeFloor final : public NodeConstArgCount<1, NodeCeil>
    {
        virtual Type type() const override
        {
            return Type::Floor;
        }
        static shared_ptr<Data> evaluate(shared_ptr<Data> retval)
        {
            if(retval->type() == Data::Type::Float)
            {
                return shared_ptr<Data>(new DataInteger((int32_t)ceil(dynamic_cast<DataFloat *>(retval.get())->value)));
            }
            if(retval->type() == Data::Type::Integer)
            {
                return shared_ptr<Data>(new DataInteger(dynamic_cast<DataInteger *>(retval.get())->value));
            }
            throw ScriptException(make_shared<DataString>(L"invalid type for ceil : " + retval->typeString()));
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
        }
    };
    struct NodeFor final : public NodeConstArgCount<4, NodeFor>
    {
        virtual Type type() const override
        {
            return Type::For;
        }
        virtual shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
        {
            checkStackDepth(stackDepth);
            shared_ptr<Data> retval;
            state.nodes[args[0]]->evaluate(state, stackDepth + 1);
            while(true)
            {
                state.onLoop();
                shared_ptr<Data> condition = state.nodes[args[1]]->evaluate(state, stackDepth + 1);
                if(condition->type() != Data::Type::Boolean)
                {
                    throw ScriptException(make_shared<DataString>(L"invalid type for conditional : " + condition->typeString()));
                }
                if(!dynamic_cast<DataBoolean *>(condition.get())->value)
                {
                    return retval;
                }
                retval = state.nodes[args[3]]->evaluate(state, stackDepth + 1);
                state.nodes[args[2]]->evaluate(state, stackDepth + 1);
            }
        }
    };

    vector<shared_ptr<Node>> nodes;
    shared_ptr<Data> evaluate() const
    {
        State state(nodes);
        return nodes.back()->evaluate(state);
    }
    Matrix evaluateAsMatrix() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Matrix)
        {
            throw ScriptException(retval);
        }
        return dynamic_pointer_cast<DataMatrix>(retval)->value;
    }
    VectorF evaluateAsVector() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Vector)
        {
            throw ScriptException(retval);
        }
        return dynamic_pointer_cast<DataVector>(retval)->value;
    }
    int32_t evaluateAsInteger() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Integer)
        {
            throw ScriptException(retval);
        }
        return dynamic_pointer_cast<DataInteger>(retval)->value;
    }
    float evaluateAsFloat() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Float)
        {
            throw ScriptException(retval);
        }
        return dynamic_pointer_cast<DataFloat>(retval)->value;
    }
    wstring evaluateAsString() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::String)
        {
            throw ScriptException(retval);
        }
        return dynamic_pointer_cast<DataString>(retval)->value;
    }
    bool evaluateAsBoolean() const
    {
        shared_ptr<Data> retval = evaluate();
        if(retval->type() != Data::Type::Boolean)
        {
            throw ScriptException(retval);
        }
        return dynamic_pointer_cast<DataBoolean>(retval)->value;
    }
    static shared_ptr<Script> parse(wstring code);
};

inline shared_ptr<Script::Data> Script::Data::read(Reader &reader)
{
    Type type = readType(reader);
    switch(type)
    {
    case Type::Boolean:
        return static_pointer_cast<Data>(DataBoolean::read(reader));
    case Type::Integer:
        return static_pointer_cast<Data>(DataInteger::read(reader));
    case Type::Float:
        return static_pointer_cast<Data>(DataFloat::read(reader));
    case Type::Vector:
        return static_pointer_cast<Data>(DataVector::read(reader));
    case Type::Matrix:
        return static_pointer_cast<Data>(DataMatrix::read(reader));
    case Type::List:
        return static_pointer_cast<Data>(DataList::read(reader));
    case Type::Object:
        return static_pointer_cast<Data>(DataObject::read(reader));
    case Type::String:
        return static_pointer_cast<Data>(DataString::read(reader));
    case Type::Last:
        break;
    }
    assert(false);
}

inline shared_ptr<Script::Node> Script::Node::read(Reader &reader, uint32_t nodeCount)
{
    Type type = readType(reader);
    switch(type)
    {
    case Type::Last:
        break;
    case Type::Const:
        return NodeConst::read(reader, nodeCount);
    case Type::CastToString:
        return NodeCastToString::read(reader, nodeCount);
    case Type::CastToInteger:
        return NodeCastToInteger::read(reader, nodeCount);
    case Type::CastToFloat:
        return NodeCastToFloat::read(reader, nodeCount);
    case Type::CastToVector:
        return NodeCastToVector::read(reader, nodeCount);
    case Type::CastToMatrix:
        return NodeCastToMatrix::read(reader, nodeCount);
    case Type::CastToList:
        return NodeCastToList::read(reader, nodeCount);
    case Type::CastToObject:
        return NodeCastToObject::read(reader, nodeCount);
    case Type::CastToBoolean:
        return NodeCastToBoolean::read(reader, nodeCount);
    case Type::LoadGlobals:
        return NodeLoadGlobals::read(reader, nodeCount);
    case Type::ReadIndex:
        return NodeReadIndex::read(reader, nodeCount);
    case Type::AssignIndex:
        return NodeAssignIndex::read(reader, nodeCount);
    case Type::Add:
        return NodeAdd::read(reader, nodeCount);
    case Type::Sub:
        return NodeSub::read(reader, nodeCount);
    case Type::Mul:
        return NodeMul::read(reader, nodeCount);
    case Type::Div:
        return NodeDiv::read(reader, nodeCount);
    case Type::Mod:
        return NodeMod::read(reader, nodeCount);
    case Type::Pow:
        return NodePow::read(reader, nodeCount);
    case Type::And:
        return NodeAnd::read(reader, nodeCount);
    case Type::Or:
        return NodeOr::read(reader, nodeCount);
    case Type::Xor:
        return NodeXor::read(reader, nodeCount);
    case Type::Concat:
        return NodeConcat::read(reader, nodeCount);
    case Type::Dot:
        return NodeDot::read(reader, nodeCount);
    case Type::Cross:
        return NodeCross::read(reader, nodeCount);
    case Type::Equal:
        return NodeEqual::read(reader, nodeCount);
    case Type::NotEqual:
        return NodeNotEqual::read(reader, nodeCount);
    case Type::LessThan:
        return NodeLessThan::read(reader, nodeCount);
    case Type::GreaterThan:
        return NodeGreaterThan::read(reader, nodeCount);
    case Type::LessEqual:
        return NodeLessEqual::read(reader, nodeCount);
    case Type::GreaterEqual:
        return NodeGreaterEqual::read(reader, nodeCount);
    case Type::Not:
        return NodeNot::read(reader, nodeCount);
    case Type::Neg:
        return NodeNeg::read(reader, nodeCount);
    case Type::Abs:
        return NodeAbs::read(reader, nodeCount);
    case Type::Sin:
        return NodeSin::read(reader, nodeCount);
    case Type::Cos:
        return NodeCos::read(reader, nodeCount);
    case Type::Tan:
        return NodeTan::read(reader, nodeCount);
    case Type::ATan:
        return NodeATan::read(reader, nodeCount);
    case Type::ASin:
        return NodeASin::read(reader, nodeCount);
    case Type::ACos:
        return NodeACos::read(reader, nodeCount);
    case Type::Exp:
        return NodeExp::read(reader, nodeCount);
    case Type::Log:
        return NodeLog::read(reader, nodeCount);
    case Type::Sqrt:
        return NodeSqrt::read(reader, nodeCount);
    case Type::ATan2:
        return NodeATan2::read(reader, nodeCount);
    case Type::Conditional:
        return NodeConditional::read(reader, nodeCount);
    case Type::MakeRotate:
        return NodeMakeRotate::read(reader, nodeCount);
    case Type::MakeRotateX:
        return NodeMakeRotateX::read(reader, nodeCount);
    case Type::MakeRotateY:
        return NodeMakeRotateY::read(reader, nodeCount);
    case Type::MakeRotateZ:
        return NodeMakeRotateZ::read(reader, nodeCount);
    case Type::MakeScale:
        return NodeMakeScale::read(reader, nodeCount);
    case Type::MakeTranslate:
        return NodeMakeTranslate::read(reader, nodeCount);
    case Type::Block:
        return NodeBlock::read(reader, nodeCount);
    case Type::ListLiteral:
        return NodeListLiteral::read(reader, nodeCount);
    case Type::NewObject:
        return NodeNewObject::read(reader, nodeCount);
    case Type::DoWhile:
        return NodeDoWhile::read(reader, nodeCount);
    case Type::RemoveTranslate:
        return NodeRemoveTranslate::read(reader, nodeCount);
    case Type::Invert:
        return NodeInvert::read(reader, nodeCount);
    case Type::Ceil:
        return NodeCeil::read(reader, nodeCount);
    case Type::Floor:
        return NodeFloor::read(reader, nodeCount);
    case Type::For:
        return NodeFor::read(reader, nodeCount);
    }
    assert(false);
}

#endif // SCRIPT_H_INCLUDED

