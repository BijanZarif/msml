# -*- encoding: utf-8 -*-
# region gplv3preamble
# The Medical Simulation Markup Language (MSML) - Simplifying the biomechanical modeling workflow
#
# MSML has been developed in the framework of 'SFB TRR 125 Cognition-Guided Surgery'
#
# If you use this software in academic work, please cite the paper:
# S. Suwelack, M. Stoll, S. Schalck, N.Schoch, R. Dillmann, R. Bendl, V. Heuveline and S. Speidel,
# The Medical Simulation Markup Language (MSML) - Simplifying the biomechanical modeling workflow,
# Medicine Meets Virtual Reality (MMVR) 2014
#
# Copyright (C) 2013-2014 see Authors.txt
#
# If you have any questions please feel free to contact us at suwelack@kit.edu
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# endregion

"""


"""

from collections import namedtuple
import re
import warnings

from path import path

from msml.exceptions import *
from ..sorts import conversion
from msml.sorts import get_sort


__author__ = "Alexander Weigl"
__date__ = "2014-01-25"


def xor(l):
    """
    xor on iterables

    >>> l1 = [0, 1 , 0]
    >>> l2 = [1, 1 , 0]
    >>> l3 = [1, 1 , 1]
    >>> xor(l1)
    True
    >>> xor(l2)
    False
    >>> xor(l3)
    True


    :returns: True if an odd numbers of True values within the ``l``
    """
    return reduce(lambda x, y: x ^ y, map(bool, l), False)


class MSMLFile(object):
    """Main model of this project.

    It holds the datastructure for an whole file.
    Consists of variables, workflow, scene and environment settings.
    It provides function for validation and reference bind between workflow, scene and variables.

    """

    def __init__(self, variables=None, workflow=None, scene=None, env=None, output=None):
        if variables:
            self._variables = {v.name: v for v in variables}
        else:
            self._variables = dict()

        self._workflow = workflow if workflow else Workflow()
        self._scene = scene if scene else []
        self._env = env if env else MSMLEnvironment()
        self._output = output if output else []
        self._exporter = None

        assert isinstance(self._env, MSMLEnvironment)

    @property
    def variables(self):
        """
        :type: dict[str, MSMLVariable]
        :return:
        """
        return self._variables

    @property
    def workflow(self):
        """
        :type: Workflow
        :return:
        """
        return self._workflow

    @property
    def scene(self):
        """
        :type: list[ SceneObject ]
        :return:
        """
        return self._scene

    @property
    def env(self):
        """
        :type: MSMLEnvironment
        :return:
        """
        return self._env

    @property
    def output(self):
        """
        :type: list[ObjectElement]
        :return:
        """
        return self._output

    @property
    def exporter(self):
        return self._exporter

    @exporter.setter
    def exporter(self, exp):
        self._exporter = exp

    def exists_var(self, name):
        """checks if variable with ``name`` exists


        :param name: name of a variable
        :type name: str

        :return:
        :rtype: bool
        """
        return name in self._variables

    def get_var(self, name):
        """ returns a variable with name if it exists

        :param name:
        :return:
        """
        return self._variables[name]

    def validate(self, alphabet=None):
        """validates the given MSMLFile.
        Delegate this to his children.

        :param alphabet:
        :rtype: bool
        :return:
        """
        if not alphabet:
            import msml.env

            alphabet = msml.env.CURRENT_ALPHABET

        b = all(call_method_list(self.scene, "bind", alphabet))

        self.workflow.bind_operators(alphabet)
        self.workflow.link(alphabet, self)
        call_method_list(self.scene, "validate")
        self.exporter.link()
        a = self.workflow.check_arguments()
        return a and b

    def lookup(self, ref, outarg=True):
        """Lookup a ``reference``.
        In ``reference`` the ``task`` has to be set. The user get a warnung if the reference is ambiguous.
        The lookup order is: task, variable, exporter.

        :param ref: an open reference 
        :type ref: Reference
        :param outarg: specifies to look for an input (+ parameters) or output slot
        :type outarg: bool
        :return: the closed reference or None
        :rtype: Reference or None
        """


        def lookup_exporter():
            return self._exporter.lookup(ref, outarg)


        def lookup_task():
            task = self._workflow.lookup(ref.task)
            if task:
                op = task.operator
                args = op.output if outarg else op.input + op.parameters
                if ref.slot:
                    # choose slot
                    return task, args[ref]
                else:
                    # choose first from output/input
                    return task, args.values()[0]
            else:
                return None

        def lookup_var():
            if ref.task in self._variables:
                return self._variables[ref.task], self._variables[ref.task]
            return None


        if isinstance(ref, str):
            ref = parse_attribute_value(ref)

        looked_up = (lookup_task(), lookup_var(), lookup_exporter())

        first_true = looked_up[0] or looked_up[1] or looked_up[2]
        if not xor(looked_up):
            # raise MSMLError("reference %s is ambigous. Found: %s" % (ref, looked_up))
            #relaxed /weigl
            warnings.warn("reference %s is ambigous. Found: %s" % (ref, looked_up))

        if not first_true:
            raise MSMLError("could not bind %s to a slot" % ref)

        return first_true


    def add_variable(self, var):
        """adds the given ``var`` to the list of variables
        This call replaces a variable with the same name.
        :param var: a new variable
        :type var: MSMLVariable
        :return: None
        """
        self._variables[var.name] = var

    def find_simulation_step(self, name):
        """Tries to find the simulation step with the given `name`

        :param name: name of the simulation step
        :type name: str
        :rtype: NoneType or msml.model.base.Environment.Simulation.Step
        """
        name = name.strip("{$}")
        for step in self.env.simulation:
            if step.name == name:
                return step
        return None


class Workflow(object):
    def __init__(self, tasks=[]):
        self._tasks = {t.id: t for t in tasks}

    def add_task(self, task):
        self._tasks[task.id] = task

    def lookup(self, id):
        return self._tasks.get(id, None)

    def bind_operators(self, alphabet):
        for t in self._tasks.values():
            t.bind(alphabet)

    def link(self, alphabet, msmlfile):
        for t in self._tasks.values():
            t.link(alphabet, msmlfile)

    def check_arguments(self):
        "checks if all tasks match the operator definition"
        return all(map(lambda x: x.validate(), self._tasks.values()))


class MSMLEnvironment(object):
    """

    .. code-block: xml

        <solver linearSolver="iterativeCG" processingUnit="CPU"
                timeIntegration="dynamicImplicitEuler"/>
        <simulation>
            <step name="initial" dt="0.05" iterations="100"/>
        </simulation>

    """

    class Simulation(list):
        class Step(object):
            def __init__(self, name="initial", dt=0.05, iterations=100):
                self.name = name
                self._dt = None
                self._iterations = None

                self.dt = dt
                self.iterations = iterations

            @property
            def dt(self):
                return self._dt

            @dt.setter
            def dt(self, dt):
                self._dt = float(dt)

            @property
            def iterations(self):
                return self._iterations

            @iterations.setter
            def iterations(self, iterations):
                self._iterations = int(iterations)


        def __init__(self, *args):
            list.__init__(self, *args)

        def add_step(self, name="initial", dt=0.05, iterations=100):
            self.append(MSMLEnvironment.Simulation.Step(name, dt, iterations))

    class Solver(object):
        def __init__(self, linearSolver="iterativeCG", processingUnit="CPU", timeIntegration="dynamicImplicitEuler",
                     preconditioner="SGAUSS_SEIDL", dampingRayleighRatioMass=0.0, dampingRayleighRatioStiffness=0.2):
            self.linearSolver = linearSolver
            self.processingUnit = processingUnit
            self.timeIntegration = timeIntegration

            self.preconditioner = preconditioner
            self.dampingRayleighRatioMass = dampingRayleighRatioMass
            self.dampingRayleighRatioStiffness = dampingRayleighRatioStiffness

    def __init__(self):
        self.simulation = MSMLEnvironment.Simulation()
        self.solver = MSMLEnvironment.Solver()


from ..log import report


class MSMLVariable(object):
    def __init__(self, name, physical=None, logical=None, value=None):
        self.name = name
        self.physical_type = physical
        self.logical_type = logical
        self.value = value

        if not self.physical_type and self.value:
            self.physical_type = type(self.value)

        if not self.physical_type:
            s = 'Try to initialize a variable without physical type and value'
            report(s, 'F', 666)
            raise MSMLError(s)

        self. \
            sort = get_sort(self.physical_type, self.logical_type)
        if not isinstance(self.value, self.sort.physical):
            report("Need convert value of %s" % self, 'I', 6161)
            from_type = type(self.value)
            converter = conversion(from_type, self.sort)
            self.value = converter(self.value)


    def __str__(self):
        return "<var %s : %s = %s>" % (self.name, self.sort, self.value)

    def __repr__(self):
        return "MSMLVariable(%s,%s,%s)" % (self.name, self.physical_type, self.logical_type)

    def __getstate__(self):
        state = self.__dict__.copy()
        del state['sort']
        return state

    def __setstate__(self, state):
        self.__dict__.update(state)
        self._find_sort()


class MSMLFileVariable(object):
    def __init__(self, name, format=None, typ=None, location=None):
        self.name = name
        self.sort = get_sort([format, type])
        self.format = format
        self.type = type
        self.location = path(location)

        if self.location is not None:
            self.type = 'file'
            self.format = "file." + self.location.ext()
            self.sort = get_sort([self.format, self.type])
        elif format is not None and type is not None:
            self.sort = get_sort([format, typ])

    def __str__(self):
        return "<Variable %s of (%s,%s)>" % (self.name, self.type, self.format)

    def __repr__(self):
        return "MSMLVariable(%s,%s,%s)" % (self.name, self.format, self.type)


class Constant(object):
    """A constant value within attribute values.

    .. note::

       ``<op-name a="c">``

       The attribute ``a`` will be coded as an :py:class:`Constant` with value ``c``.

    This class will be created py :py:func:`parse_attribute_value`

    .. seealso::

        :py:func:`parse_parse_attribute_value`

    """

    def __init__(self, value):
        self._value = value

    @property
    def value(self):
        """the value of this constant, **readonly**"""
        return self._value


class Reference(object):
    """Describes a connection from an *input slot* to an *output slot*.
    """
    ref = namedtuple("ref", "task,name,arginfo")

    def __init__(self, task, out=None):
        self.task = task
        self.slot = out

        self.linked_from = None
        self.linked_to = None

    @property
    def linked(self):
        """ Returns True iff. the Reference is closed iff. input and output slot found and set.
        :return: bool
        """
        return self.linked_to is not None and self.linked_from is not None

    @property
    def valid(self):
        """ Returns True iff. the input and output slot are physical type compatible
        :return: bool
        """
        return self.linked and self.linked_from.arginfo.sort <= self.linked_to.arginfo.sort

    def link_from_task(self, task, slot):
        self.linked_from = Reference.ref(task, slot.name, slot)

    def link_from_variable(self, variable):
        self.linked_from = Reference.ref(variable, variable.name, variable)

    def link_to_task(self, task, slot):
        self.linked_to = Reference.ref(task, slot.name, slot)


    def __str__(self):
        def _id(a):
            try:
                return a.id
            except:
                return a.name

        if self.linked:
            return "<Reference+: %s -> %s>" % (self.linked_from.arginfo, self.linked_to.arginfo)

        else:
            return "<Reference-: %s.%s>" % (self.task, self.slot)


def parse_attribute_value(value):
    """Parse attribute values:
    >>> parse_attribute_value("${abc}") # => Reference
    >>> parse_attribute_value("abc") # => Constant

    :param str value: value of a xml attribute
    :return: Constant | Reference
    """
    if isinstance(value, str):
        expr = re.match(r'\${([^.]+)(\.[^.]+)?}', value)
        if expr:
            a = expr.group(1).strip(".")
            try:
                b = expr.group(2).strip(".")
            except:
                b = None
            return Reference(a, b)
    return Constant(value)


def random_var_name():
    """generate a random variable name
    :return: str
    """
    random_var_name.current_number += 1
    return "_gen_%03d_" % random_var_name.current_number


random_var_name.current_number = 0  # start with zero


def is_generated_name(name):
    """True iff. the given `name` is a generated name
    :param str name:
    :return: bool
    """
    return name.startswith("_gen_")


class Task(object):
    """

    """
    ID_ATTRIB = "id"

    def __init__(self, name, attrib):
        self.name = name
        self.id = attrib[Task.ID_ATTRIB]
        del attrib[Task.ID_ATTRIB]

        self.attributes = {k: parse_attribute_value(value) for k, value in attrib.items()}
        self.operator = None
        self.arguments = {}

    def __str__(self):
        return "{Task %s (%s)}" % (self.id, self.name)

    def bind(self, alphabet):
        self.operator = alphabet.get(self.name)

    def link(self, alphabet, msmlfile):
        self.arguments = {}
        for key, value in self.attributes.items():
            if isinstance(value, Reference):
                a = msmlfile.lookup(value)
                if a:
                    outtask, outarg = a
                    value.link_from_task(outtask, outarg)
                    try:
                        if key in self.operator.input:
                            value.link_to_task(self, self.operator.input[key])
                        else:
                            value.link_to_task(self, self.operator.parameters[key])
                    except KeyError, e:

                        f = str(a)
                        t = str(self.name)
                        i = key
                        op = str(self.operator)
                        inputs = ",".join(map(str, self.operator.acceptable_names()))

                        raise BindError(
                            "you try to connect {start} to Task '{target}' but slot {inputname} is unknown for {operator} (Inputs {inputs})".format(
                                start=f, target=t, inputname=i, operator=op, inputs=inputs
                            ))
                    except AttributeError:
                        raise MSMLError(
                            "the operator for Task {target} is {operator} ".format(
                                target=self.name, operator=self.operator
                            ))

                    self.arguments[key] = value
                else:
                    report("Lookup after %s does not succeeded" % value, 'E')
            elif isinstance(value, Constant):
                var = MSMLVariable(random_var_name(), value=value.value)
                # get type and format from input/parameter

                slot = self.operator.input.get(key, None) or self.operator.parameters.get(key, None)
                if slot:
                    var.logical_type = slot.logical_type
                    var.physical_type = slot.physical_type

                msmlfile.add_variable(var)
                ref = Reference(var.name, None)
                outtask, outarg = msmlfile.lookup(ref)
                ref.link_from_task(outtask, outarg)

                try:
                    # ref.link_to_task(self, self.operator.input[key])
                    if key in self.operator.input:
                        ref.link_to_task(self, self.operator.input[key])
                    else:
                        ref.link_to_task(self, self.operator.parameters[key])
                except KeyError, e:
                    f = str(var)
                    t = str(self.name)
                    i = key
                    op = str(self.operator)
                    inputs = ",".join(map(str, self.operator.acceptable_names()))

                    raise BindError(
                        "you try to connect {start} to Task '{target}' but slot {inputname} is unknown for {operator} (Inputs {inputs})".format(
                            start=f, target=t, inputname=i, operator=op, inputs=inputs
                        ))

                self.arguments[key] = ref
            else:
                raise MSMLError("no case %s : %s for attribute %s in %s " % (value, str(type(value)), key, self))

    def validate(self):
        if not self.operator:
            raise BindError("self.operator is not bound to an operator (call Workflow.bind_operator?)")

            # if self.id:
            # raise MSMLError("Task does not have an <id> attribute")

            # for arg in self.arguments.values():
            #    if arg.inarg.required and arg.outarg is None:
            #        raise MSMLError("task for operator %s misses attribute %s " % (self.operator.name, arg.attribute))

            #        for k in self.attrib:
            #            if k not in self.operator and k != Task.ID_ATTRIB:
            #                raise MSMLError("attrib %s is unknown for operator %s" % (k, self.operator.name))

    def get_default(self):
        pass


class SceneObjectSets(object):
    """

    """

    def __init__(self, elements=None, nodes=None, surfaces=None):
        self.elements = elements
        self.nodes = nodes
        self.surfaces = surfaces


def call_method_list(seq, method, *args):
    return map(lambda element: getattr(element, method)(*args), seq)


class SceneObject(object):
    """

    """

    def __init__(self, oid, mesh=None, sets=SceneObjectSets(), material=None, constraints=None):
        self._id = oid
        self._mesh = mesh if mesh else Mesh()
        self._material = list() if not material else material
        self._constraints = constraints if constraints else list()
        self._sets = sets
        self._output = list()

    def bind(self, alphabet):
        """

        :return:
        """
        call_method_list(self.material, 'bind', alphabet)
        call_method_list(self.constraints, 'bind', alphabet)
        call_method_list(self.output, 'bind', alphabet)

    def validate(self):
        """
        :return:
        :rtype: bool
        """

        a = self.mesh.validate()
        b = call_method_list(self.material, 'validate')
        c = call_method_list(self.constraints, 'validate')
        d = call_method_list(self.output, 'validate')

        return all([a] + b + c + d)

    @property
    def id(self):
        return self._id

    @id.setter
    def id(self, value):
        self._id = value

    @property
    def sets(self):
        return self._sets

    @sets.setter
    def sets(self, value):
        self._sets = value

    @property
    def mesh(self):
        """
        :type: Mesh
        :return:
        """
        return self._mesh

    @mesh.setter
    def mesh(self, value):
        assert isinstance(value, Mesh)
        self._mesh = value

    @property
    def material(self):
        return self._material

    @material.setter
    def material(self, value):
        self._material = value

    @property
    def output(self):
        return self._output

    @output.setter
    def output(self, value):
        self._output = value

    @property
    def constraints(self):
        return self._constraints

    @constraints.setter
    def constraints(self, c):
        self._constraints = c

    @property
    def material(self):
        return self._material

    @material.setter
    def material(self, mat):
        self._material = mat


class ObjectElement(object):
    """This class describe an instance of an :py:class:`ObjectAttribute`

    """

    def __init__(self, attrib={}, meta=None):
        self.attributes = attrib
        """a dictionary of the given xml attributes
        :type: dict
        """
        self.meta = meta
        """the given ObjectAttribute template
        :type: ObjectAttribute
        """

    def __getattr__(self, item):
        return self.get(item, None)

    def bind(self, alphabet=None):
        if not alphabet:
            import msml.env

            alphabet = msml.env.CURRENT_ALPHABET

        self.meta = alphabet.get(self.__tag__)

    def validate(self):
        """

        :return:
        """
        b, c = True, True
        if self.meta is None:
            raise MSMLError("for %s is no meta defined. The element %s is not part of the givne MSML Alphabet" % (
            self.__tag__, self.__tag__))

        for key, value in self.attributes.items():
            if key == "__tag__":
                continue

            if key not in self.meta.parameters:
                report("Parameter %s of Element %s is not specified in definition." % (key, self.meta.name), 'E')
                b = False

        for key, value in self.meta.parameters.items():
            if key not in self.attributes and value.required:
                report("Parameter %s of Definiton %s is not specified in msml file." % (key, self.id or self.meta.name),
                       'F')
                c = False

        return b and c

    @property
    def tag(self):
        """:return: the element (tag) name, this is the same as the `ObjectAttribute.name` in `self.meta`
        """
        return self.attributes['__tag__']

    @tag.setter
    def tag(self, tag):
        self.attributes['__tag__'] = tag

    @property
    def meta(self):
        return self._meta

    @meta.setter
    def meta(self, new):
        self._meta = new

    def _get(self, attribute):
        """

        :param attribute:
        :return:
        """
        if attribute in self.attributes:
            return self.attributes[attribute]
        else:
            if self.meta:
                return self.meta.parameters[attribute].default
        raise KeyError("Could not find %s in ObjectElement attributes or a parameter default")

    def get(self, attribute, default=None):
        """

        :param attribute:
        :param default:
        :return:
        """
        try:
            return self._get(attribute)
        except KeyError:
            return default


class ObjectConstraints(object):
    """Constraints for a timestep (``for_step``)
    """

    def __init__(self, name, forStep="initial"):
        self._name = name
        self._forStep = forStep
        self._constraints = []

    def bind(self, alphabet):
        """binds all constraints to the given ``alphabet``
        :type alphabet: msml.model.alphabet.Alphabet
        :return: None
        """
        call_method_list(self.constraints, 'bind', alphabet)

    def validate(self):
        """validates all constraints
        :return: True iff. all constraints valid, and ``for_step`` is set
        """
        return all(map(lambda x: x.validate(), self.constraints))

    @property
    def index_group(self):
        warn(DeprecationWarning, "This method will be removed at the next refactoring! /alexander weigl")
        for c in self._constraints:
            if c.attributes['__tag__'] == 'indexgroup': return c
        return None

    @property
    def name(self):
        """
        :type: str
        :return:
        """
        return self._name

    @name.setter
    def name(self, v):
        self._name = v

    @property
    def for_step(self):
        """
        :type: str
        :return:
        """
        return self._forStep

    @for_step.setter
    def for_step(self, v):
        self._forStep = v

    @property
    def constraints(self):
        """
        :type: list[ObjectElement]
        :return:
        """
        return self._constraints

    @constraints.setter
    def constraints(self, v):
        self._constraints = v

    def add_constraint(self, *constraints):
        """add ``constraints`` to this object
        ;type constraints: ObjectElement
        :param constraints:
        :return:
        """
        self._constraints += constraints


class SceneSets(object):
    """Represents the sets with an :py:class:`SceneObject`
    """

    def __init__(self, nodes=None, surfaces=None, elements=None):
        self.nodes = nodes or list()
        self.surfaces = surfaces or list()
        self.elements = elements or list()


class IndexGroup(object):
    """The indexgroup element

    .. code-block:: xml

        <indexgroup id="" indices="" />

    """

    def __init__(self, id, indices):
        self.id = id
        self.indices = indices


class Mesh(object):
    """Represent the given mesh within the <object> node:

    .. code-block:: xml

        <mesh>
            <*type* id="" mesh="" />
        <mesh>


    """

    def __init__(self, type="linear", id=None, value=None):
        """
        :param str type: type of the mesh (one of ``linear``, ``quadratic``)
        :param str id: id of the mesh
        :param str value: value of the mesh (a reference or a reference string)
        """
        self.type = type
        self.id = id
        self.value = value

    @property
    def mesh(self):
        """
        legacy support

        .. deprecated::

            use ``self.value``

        """
        return self.value

    def validate(self):
        """
        :return: always valid
        """
        return True


class MaterialRegion(IndexGroup, list):
    """Represents an material region from an MSMLFile within an SceneObject

    .. code-block:: xml

        <material>
            <region id="" indices="">
                [object elements]
            </region>
        </material>

    .. seealso::

       :py:class:`SceneObject`

    """

    def __init__(self, id, indices, elements=None):
        IndexGroup.__init__(self, id, indices)
        list.__init__(self, elements if elements else [])

    def bind(self, alphabet):
        """binds all sub elements to the corresponding :py:class:`msml.model.alphabet.ObjectAttribute`

        :param alphabet: the current alphabet
        :type alphabet: msml.model.alphabet.Alphabet
        :return: None
        """
        call_method_list(self, 'bind', alphabet)

    def validate(self):
        """
        :type: bool
        :return: True iff. all sub elements are valid and the region is valid.
        """
        b = self.indices is not None and self.indices != ""
        if not b:
            report("MaterialRegion has no id value", 'E')

        a = self.indices is not None and self.indices != ""
        if not a:
            report("MaterialRegion %s has no indices" % self.id, 'E')
        return a and b and all(map(lambda x: x.validate(), self))