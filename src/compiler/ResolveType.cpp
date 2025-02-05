#include "fast_ber/compiler/ResolveType.hpp"

#include <assert.h>

Module& find_module(Asn1Tree& tree, const std::string& module_reference)
{
    for (Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            return module;
        }
    }
    throw std::runtime_error("Module does not exist " + module_reference);
}

const Module& find_module(const Asn1Tree& tree, const std::string& module_reference)
{
    for (const Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            return module;
        }
    }
    throw std::runtime_error("Module does not exist " + module_reference);
}

Module& find_module(Asn1Tree& tree, Module& current_module, const DefinedType& defined)
{
    const std::string& module_reference =
        (defined.module_reference) ? *defined.module_reference : current_module.module_reference;

    for (Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            for (const Assignment& assignment : module.assignments)
            {
                if (assignment.name == defined.type_reference)
                {
                    return module;
                }
            }
        }
    }

    throw std::runtime_error("Reference to undefined object: " + current_module.module_reference + "." +
                             defined.type_reference);
}

const Module& find_module(const Asn1Tree& tree, const Module& current_module, const DefinedType& defined)
{
    const std::string& module_reference =
        (defined.module_reference) ? *defined.module_reference : current_module.module_reference;

    for (const Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            for (const Assignment& assignment : module.assignments)
            {
                if (assignment.name == defined.type_reference)
                {
                    return module;
                }
            }
        }
    }

    throw std::runtime_error("Reference to undefined object: " + current_module.module_reference + "." +
                             defined.type_reference);
}

Assignment& resolve(Asn1Tree&, Module& module, const std::string& reference)
{
    for (Assignment& assignemnt : module.assignments)
    {
        if (assignemnt.name == reference)
        {
            return assignemnt;
        }
    }

    throw std::runtime_error("Reference to undefined object: " + module.module_reference + "." + reference);
}

Assignment& resolve(Asn1Tree& tree, const std::string& module_reference, const std::string& reference)
{
    for (Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            return resolve(tree, module, reference);
        }
    }
    throw std::runtime_error("Reference to undefined object: " + module_reference + "." + reference);
}

Assignment& resolve(Asn1Tree& tree, const std::string& current_module_reference, const DefinedType& defined)
{
    const std::string& module_reference =
        (defined.module_reference) ? *defined.module_reference : current_module_reference;

    for (Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            return resolve(tree, module, defined.type_reference);
        }
    }
    throw std::runtime_error("Reference to undefined object: " + module_reference + "." + defined.type_reference);
}

const Assignment& resolve(const Asn1Tree&, const Module& module, const std::string& reference)
{
    for (const Assignment& assignemnt : module.assignments)
    {
        if (assignemnt.name == reference)
        {
            return assignemnt;
        }
    }

    throw std::runtime_error("Reference to undefined object: " + module.module_reference + "." + reference);
}

const Assignment& resolve(const Asn1Tree& tree, const std::string& module_reference, const std::string& reference)
{
    for (const Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            return resolve(tree, module, reference);
        }
    }
    throw std::runtime_error("Reference to undefined object: " + module_reference + "." + reference);
}

const Module& resolve_module(const Asn1Tree& tree, const std::string& current_module_reference,
                             const DefinedType& defined)
{
    const std::string& module_reference =
        (defined.module_reference) ? *defined.module_reference : current_module_reference;

    for (const Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            return module;
        }
    }
    throw std::runtime_error("Reference to undefined object: " + module_reference + "." + defined.type_reference);
}

const Assignment& resolve(const Asn1Tree& tree, const std::string& current_module_reference, const DefinedType& defined)
{
    return resolve(tree, resolve_module(tree, current_module_reference, defined), defined.type_reference);
}

NamedTypeAndModule resolve_type_and_module(const Asn1Tree& tree, const std::string& current_module_reference,
                                           const DefinedType& original_defined)
{
    DefinedType defined = original_defined;
    std::string module  = current_module_reference;
    while (true)
    {
        assert(std::isupper(defined.type_reference[0]));
        const Module&     working_module = resolve_module(tree, module, defined);
        const Assignment& assignment     = resolve(tree, working_module, defined.type_reference);
        if (!is_type(assignment))
        {
            throw std::runtime_error("Defined must be a type in this context! " + original_defined.type_reference);
        }
        if (is_defined(type(assignment)))
        {
            if (defined.module_reference)
            {
                module = *defined.module_reference;
            }
            defined = std::get<DefinedType>(type(assignment));
        }
        else if (is_prefixed(type(assignment)))
        {
            PrefixedType prefixed = std::get<PrefixedType>(std::get<BuiltinType>(type(assignment)));
            if (is_defined(prefixed.tagged_type->type))
            {
                NamedTypeAndModule const& inner = resolve_type_and_module(
                    tree, current_module_reference, std::get<DefinedType>(prefixed.tagged_type->type));
                prefixed.tagged_type->type = inner.type.type;
                return {NamedType{assignment.name, prefixed}, inner.module};
            }
            else
            {
                return {NamedType{assignment.name, type(assignment)}, working_module};
            }
        }
        else
        {
            return {NamedType{assignment.name, type(assignment)}, working_module};
        }
    }
}

NamedTypeAndModule resolve_type_and_module(const Asn1Tree& tree, const std::string& current_module_reference,
                                           const NamedType& type_info)
{
    if (is_defined(type_info.type))
    {
        return resolve_type_and_module(tree, current_module_reference, std::get<DefinedType>(type_info.type));
    }
    if (is_prefixed(type_info.type))
    {
        return resolve_type_and_module(
            tree, current_module_reference,
            NamedType{type_info.name,
                      std::get<PrefixedType>(std::get<BuiltinType>(type_info.type)).tagged_type->type});
    }

    return {type_info, find_module(tree, current_module_reference)};
}

NamedType resolve_type(const Asn1Tree& tree, const std::string& current_module_reference,
                       const DefinedType& original_defined)
{
    return resolve_type_and_module(tree, current_module_reference, original_defined).type;
}

NamedType resolve_type(const Asn1Tree& tree, const std::string& current_module_reference, const NamedType& type_info)
{
    return resolve_type_and_module(tree, current_module_reference, type_info).type;
}

bool exists(const Asn1Tree&, const Module& module, const std::string& reference)
{
    for (const Assignment& assignemnt : module.assignments)
    {
        if (assignemnt.name == reference)
        {
            return true;
        }
    }

    return false;
}

bool exists(const Asn1Tree& tree, const std::string& module_reference, const std::string& reference)
{
    for (const Module& module : tree.modules)
    {
        if (module.module_reference == module_reference)
        {
            return exists(tree, module, reference);
        }
    }
    return false;
}

bool exists(const Asn1Tree& tree, const std::string& current_module_reference, const DefinedType& defined)
{
    const std::string& module_reference =
        (defined.module_reference) ? *defined.module_reference : current_module_reference;

    return exists(tree, module_reference, defined.type_reference);
}

bool is_type(const Assignment& assignment) { return std::holds_alternative<TypeAssignment>(assignment.specific); }
bool is_value(const Assignment& assignment) { return std::holds_alternative<ValueAssignment>(assignment.specific); }
bool is_object_class(const Assignment& assignment)
{
    return std::holds_alternative<ObjectClassAssignment>(assignment.specific) ||
           std::holds_alternative<ObjectSetAssignment>(assignment.specific);
}

Type&       type(Assignment& assignemnt) { return std::get<TypeAssignment>(assignemnt.specific).type; }
const Type& type(const Assignment& assignemnt) { return std::get<TypeAssignment>(assignemnt.specific).type; }

ValueAssignment&       value(Assignment& assignemnt) { return std::get<ValueAssignment>(assignemnt.specific); }
const ValueAssignment& value(const Assignment& assignemnt) { return std::get<ValueAssignment>(assignemnt.specific); }

ObjectClassAssignment& object_class(Assignment& assignemnt)
{
    return std::get<ObjectClassAssignment>(assignemnt.specific);
}
const ObjectClassAssignment& object_class(const Assignment& assignemnt)
{
    return std::get<ObjectClassAssignment>(assignemnt.specific);
}

bool is_a_parameter(const std::string& reference, const std::vector<Parameter>& parameters)
{
    auto parameter_match = [&](const Parameter& param) { return param.reference == reference; };

    return std::find_if(parameters.begin(), parameters.end(), parameter_match) != parameters.end();
}
