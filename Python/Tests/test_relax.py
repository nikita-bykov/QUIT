from pathlib import Path
from os import chdir
import unittest
from nipype.interfaces.base import CommandLine
from qipype.commands import NewImage, Diff
from qipype.fitting import Multiecho, MultiechoSim

vb = True
CommandLine.terminal_output = 'allatonce'


class Relax(unittest.TestCase):
    def setUp(self):
        Path('testdata').mkdir(exist_ok=True)
        chdir('testdata')

    def tearDown(self):
        chdir('../')

    def test_multiecho(self):
        me = {'MultiEcho': {'TR': 10, 'TE1': 0.01, 'ESP': 0.01,
                            'ETL': 5}}
        me_file = 'sim_me.nii.gz'
        img_sz = [32, 32, 32]
        noise = 0.001

        NewImage(img_size=img_sz, grad_dim=0, grad_vals=(0.8, 1.0),
                 out_file='PD.nii.gz', verbose=vb).run()
        NewImage(img_size=img_sz, grad_dim=2, grad_vals=(0.04, 0.1),
                 out_file='T2.nii.gz', verbose=vb).run()

        MultiechoSim(sequence=me, out_file=me_file,
                     PD_map='PD.nii.gz', T2_map='T2.nii.gz',
                     noise=noise, verbose=vb).run()
        Multiecho(sequence=me, in_file=me_file, verbose=vb).run()

        diff_T2 = Diff(in_file='ME_T2.nii.gz', baseline='T2.nii.gz',
                       noise=noise, verbose=vb).run()
        diff_PD = Diff(in_file='ME_PD.nii.gz', baseline='PD.nii.gz',
                       noise=noise, verbose=vb).run()
        self.assertLessEqual(diff_T2.outputs.out_diff, 3)
        self.assertLessEqual(diff_PD.outputs.out_diff, 2)


if __name__ == '__main__':
    unittest.main()
