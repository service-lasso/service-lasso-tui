"""Source-level fixtures, not native or Core authority/acceptance receipts."""
import json
import os
import pathlib
import runpy
import tempfile
import types
import unittest
import sys
from unittest.mock import patch

def load_harness():
    script=str(pathlib.Path(__file__).with_name('tui20-native-posix-five-action.py'))
    if sys.platform=='linux':return runpy.run_path(script)
    with patch.dict(sys.modules,{name:types.ModuleType(name) for name in ('fcntl','pty','termios')}):return runpy.run_path(script)
HARNESS=load_harness()

@unittest.skipUnless(sys.platform=='linux','requires Linux private-file identity checks')
class OperationObserverTests(unittest.TestCase):
    def state(self,root,records):
        p=pathlib.Path(root,'workspace','.service-lasso','mcp','operations.json');p.parent.mkdir(parents=True,exist_ok=True)
        p.write_text(json.dumps({'version':1,'protection':'owner-only-file','payload':{'version':1,'operations':records}}));p.chmod(0o600);return p
    def row(self,identifier='operation'):
        return {'operationId':identifier,'actorId':'tui20-native-operator','clientId':'tui20-native-client'}
    def test_observes_identity_only_and_denies_disappearing_committed_ids(self):
        with tempfile.TemporaryDirectory() as root:
            observer=HARNESS['OperatorOperationObserver'](root);self.assertEqual(observer.ids(),set())
            p=self.state(root,[self.row()]);self.assertEqual(observer.ids(),{'operation'})
            self.state(root,[])
            with self.assertRaisesRegex(RuntimeError,'identity disappeared'):observer.ids()
            p.unlink()
            with self.assertRaisesRegex(RuntimeError,'state disappeared'):observer.ids()
    def test_corrupt_foreign_duplicate_and_unprotected_state_never_becomes_empty(self):
        for boundary in ('corrupt','wrapper','payload','actor','duplicate','permissions','symlink'):
            with self.subTest(boundary=boundary),tempfile.TemporaryDirectory() as root:
                row=self.row();p=self.state(root,[row]);observer=HARNESS['OperatorOperationObserver'](root)
                if boundary=='corrupt':p.write_text('{')
                elif boundary=='wrapper':p.write_text(json.dumps({'version':1,'protection':'unexpected','payload':{'version':1,'operations':[]}}))
                elif boundary=='payload':p.write_text(json.dumps({'version':1,'protection':'owner-only-file','payload':{'version':2,'operations':[]}}))
                elif boundary=='actor':row['actorId']='foreign';self.state(root,[row])
                elif boundary=='duplicate':self.state(root,[row,row])
                elif boundary=='permissions':p.chmod(0o644)
                else:
                    target=p.with_name('actual');p.rename(target);p.symlink_to(target)
                with self.assertRaises((RuntimeError,OSError)):observer.ids()

class TerminalWaitTests(unittest.TestCase):
    def test_final_read_marker_precedes_child_exit_check_but_absence_still_fails(self):
        for observed in (True,False):
            polls=[]
            term=types.SimpleNamespace(text='',child=types.SimpleNamespace(poll=lambda:polls.append(True) or 0))
            term.read=lambda:setattr(term,'text','safe diagnostic' if observed else 'other bytes')
            wait=HARNESS['Terminal'].wait
            if observed:wait(term,('safe diagnostic',),1);self.assertEqual(polls,[])
            else:
                with self.assertRaisesRegex(RuntimeError,'exited before expected'):wait(term,('safe diagnostic',),1)
                self.assertEqual(polls,[True])

class OperatorKeyboardTests(unittest.TestCase):
    def test_real_macro_reviews_exact_spaced_phrase_before_one_enter_and_readback(self):
        phrase='confirm service-install 0123456789abcdef0123456789abcdef'
        keys=[];ids=set();submitted=[]
        operation={'operationId':'operation','actionId':'service_install','serviceId':'tui20-fixture','targets':['tui20-fixture'],'status':'succeeded','outcome':'succeeded','cancellationSupported':False,'correlationId':'correlation'}
        def request(url,token,path):
            self.assertEqual((url,token),('actual-url','actual-token'))
            if path.endswith('/lifecycle-availability'):return {'data':{'capabilities':[{'id':'service_install','availability':{'available':True}}]}}
            self.assertEqual(path,'/api/operator/v2/operations/operation');return {'data':{'operation':operation}}
        class Term:
            text='';phase='rows';typed=''
            def write(self,value):
                keys.append(value)
                if value=='r':self.text+='Form: available\n'
                elif value=='\x1b[H':pass
                elif value=='\x15':self.typed=''
                elif value=='\r':
                    if self.phase=='rows':self.phase='draft';self.text+='Draft: service_install\n'
                    elif self.phase=='draft':self.phase='preview';self.text+='Original preview targets:\n  tui20-fixture\nOriginal effects:\n  install_artifacts\nConfirmation phrase: '+phrase+'\n'
                    else:
                        if self.typed!=phrase:raise AssertionError('incorrect confirmation submitted')
                        self.phase='submitted';submitted.append(True);ids.add('operation');self.text+='Operation operation succeeded\n'
                else:self.typed+=value
                return len(value)
            def wait(self,need,seconds,start=0):
                if not all(value in self.text[start:] for value in need):raise AssertionError('missing fresh keyboard state')
            def read(self,*args):return None
        term=Term();observer=types.SimpleNamespace(ids=lambda:set(ids))
        function=HARNESS['operator_action']
        with patch.dict(function.__globals__,{'request':request,'diagnostic_stage':lambda label:None}),patch.object(HARNESS['time'],'sleep',lambda seconds:None):
            row=function(term,'service_install','install','actual-url','actual-token',observer)
        self.assertEqual(row['operationId'],'operation');self.assertEqual(submitted,[True])
        self.assertEqual(keys,['r','\x1b[H','\r','\r','rjq','\x15',phrase,'\r'])
        self.assertEqual(term.action_phase,'readback-complete')

class ActualAuditTests(unittest.TestCase):
    def test_denied_profile_requires_actual_seven_client_action_reason_events(self):
        subjects={'service_install','service_configure','service_start','service_stop','service_restart','update_check','update_download'}
        rows=[{'id':str(i),'action':'mcp.action.denied','actor':'tui20-native-operator','subject':action,'statusCode':403,'reason':'insufficient_profile','metadata':{'clientId':'tui20-native-denied-client','action':action}} for i,action in enumerate(sorted(subjects))]
        check=HARNESS['operator_assert_denials'];check([],rows,subjects)
        for boundary in ('count','reason','client','subject','status'):
            with self.subTest(boundary=boundary):
                altered=json.loads(json.dumps(rows))
                if boundary=='count':altered.pop()
                elif boundary=='reason':altered[0]['reason']='dependency_blocked'
                elif boundary=='client':altered[0]['metadata']['clientId']='foreign'
                elif boundary=='subject':altered[0]['subject']='foreign'
                else:altered[0]['statusCode']=503
                with self.assertRaisesRegex(RuntimeError,'permission denial audit mismatch'):check([],altered,subjects)
    def test_success_audit_requires_once_actual_correlation_target_and_actor(self):
        record={'operationId':'operation','action':'service_start','targetIds':['tui20-fixture'],'correlationId':'correlation'}
        event={'action':'mcp.operation.succeeded','actor':'tui20-native-operator','subject':'operation','correlationId':'correlation','metadata':{'operationId':'operation','action':'service_start','targetIds':['tui20-fixture']}}
        check=HARNESS['operator_success_audits'];self.assertEqual(check([record],[event]),[{'operationId':'operation','coreAuditCount':1}])
        for boundary in ('missing','duplicate','correlation','actor','target'):
            with self.subTest(boundary=boundary):
                altered=json.loads(json.dumps(event));events=[altered]
                if boundary=='missing':events=[]
                elif boundary=='duplicate':events.append(altered.copy())
                elif boundary=='correlation':altered['correlationId']='foreign'
                elif boundary=='actor':altered['actor']='foreign'
                else:altered['metadata']['targetIds']=['foreign']
                with self.assertRaisesRegex(RuntimeError,'correlated Core audit mismatch'):check([record],events)

if __name__=='__main__':unittest.main()
