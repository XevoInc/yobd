'''
A format checker that checks for strings that are valid infix notation. Four
variables are allowed (A, B, C, D), which stand in for data that will be
manipulated after the schema is parsed. Below are a few valid examples:
A * B
A + 255.0
(A - B) / 10
'''

import ast
import jsonschema
import operator
import simpleeval


FORMAT_NAME = 'infix'


def check(val):
    '''Returns True if val is a valid infix expression and raises FormatError
    otherwise.'''
    operators = {
        ast.Add: operator.add,
        ast.Sub: operator.sub,
        ast.Mult: operator.mul,
        ast.Div: operator.floordiv,
    }

    # We don't actually care what the names evaluate to, as we're not using
    # SimpleEval() to actually evaluate anything; we're just using it as a
    # shortcut to check whether or not the expression is valid in-fix. So, all
    # the values given here are just dummy placeholders.
    names = {
        'A': 0,
        'B': 0,
        'C': 0,
        'D': 0,
    }
    infix_eval = simpleeval.SimpleEval(operators=operators, names=names)

    try:
        infix_eval.eval(val)
    except:  # noqa: E722
        # eval can throw anything the Python parser can throw, so it's hard to
        # catch every possible item. So, exempt ourselves from this particular
        # flake8 warning.
        raise jsonschema.exceptions.FormatError('"%s" is not valid infix'
                                                % val)

    return True
