#pragma once

#include "fast_ber/compiler/GenerateHelpers.hpp"

template <typename Function>
CodeBlock visit_all_types(const Asn1Tree& tree, const Module& module, const Type& type,
                          const std::string& context_namespace, Function&& func)
{
    CodeBlock block;

    if (is_sequence(type))
    {
        const SequenceType& sequence = std::get<SequenceType>(std::get<BuiltinType>(type));
        for (const ComponentType& component : sequence.components)
        {
            const std::string name = context_namespace + "::" + make_type_name(component.named_type.name);
            block.add_block(
                visit_all_types(tree, module, component.named_type.type, name, std::forward<Function>(func)));
        }
    }
    else if (is_set(type))
    {
        const SetType& set = std::get<SetType>(std::get<BuiltinType>(type));
        for (const ComponentType& component : set.components)
        {
            const std::string name = context_namespace + "::" + make_type_name(component.named_type.name);
            block.add_block(
                visit_all_types(tree, module, component.named_type.type, name, std::forward<Function>(func)));
        }
    }
    else if (is_sequence_of(type))
    {
        const SequenceOfType& sequence = std::get<SequenceOfType>(std::get<BuiltinType>(type));
        if (sequence.has_name)
        {
            const std::string name = context_namespace + "::" + make_type_name(sequence.named_type->name);
            block.add_block(
                visit_all_types(tree, module, sequence.named_type->type, name, std::forward<Function>(func)));
        }
    }
    else if (is_set_of(type))
    {
        const SetOfType& set = std::get<SetOfType>(std::get<BuiltinType>(type));
        if (set.has_name)
        {
            const std::string name = context_namespace + "::" + make_type_name(set.named_type->name);
            block.add_block(visit_all_types(tree, module, set.named_type->type, name, std::forward<Function>(func)));
        }
    }
    else if (is_choice(type))
    {
        const ChoiceType& choice = std::get<ChoiceType>(std::get<BuiltinType>(type));
        for (const NamedType& named_type : choice.choices)
        {
            const std::string name = context_namespace + "::" + make_type_name(named_type.name);
            block.add_block(visit_all_types(tree, module, named_type.type, name, std::forward<Function>(func)));
        }
    }
    else if (is_prefixed(type))
    {
        const PrefixedType& prefixed = std::get<PrefixedType>(std::get<BuiltinType>(type));
        block.add_block(
            visit_all_types(tree, module, prefixed.tagged_type->type, context_namespace, std::forward<Function>(func)));
    }

    block.add_block(func(tree, module, type, context_namespace));
    return block;
}

template <typename Function>
CodeBlock visit_all_types(const Asn1Tree& tree, const Module& module, const Assignment& assignment, Function&& func)
{
    if (std::holds_alternative<TypeAssignment>(assignment.specific))
    {
        const TypeAssignment& type_assignment = std::get<TypeAssignment>(assignment.specific);
        return visit_all_types(tree, module, type_assignment.type,
                               "fast_ber::" + module.module_reference + "::" + assignment.name,
                               std::forward<Function>(func));
    }
    return {};
}
