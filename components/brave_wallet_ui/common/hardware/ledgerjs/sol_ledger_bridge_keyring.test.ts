/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { BraveWallet } from '../../../constants/types'
import SolanaLedgerKeyring from './sol_ledger_bridge_keyring'
import * as bs58 from 'bs58'
// import { SignatureVRS } from '../../hardware_operations'
// import { LedgerDerivationPaths } from '../types'

class MockApp {
  // signature: SignatureVRS

  async getAddress (path: string) {
    return { address: Buffer.from(`address for ${path}`) }
  }

  // async signPersonalMessage (path: string, message: Buffer) {
  //   return Promise.resolve(this.signature)
  // }

  // async signEIP712HashedMessage (path: string, domainSeparatorHex: string, hashStructMessageHex: string) {
  //   return Promise.resolve(this.signature)
  // }
}

const createSolanaLedgerKeyring = (app: MockApp = new MockApp()) => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerKeyring()
  solanaLedgerHardwareKeyring.unlock = async () => {
    solanaLedgerHardwareKeyring.app = app
    solanaLedgerHardwareKeyring.deviceId = 'device1'
    return { success: true }
  }

  return solanaLedgerHardwareKeyring
}

// TODO(nvonpentz) - Use in future tests
// const unlockedLedgerKeyring = () => {
//   const solanaLedgerHardwareKeyring = new SolanaLedgerKeyring()
//   solanaLedgerHardwareKeyring.app = new MockApp()
//   solanaLedgerHardwareKeyring.deviceId = 'device1'
//   return solanaLedgerHardwareKeyring
// }

test('Extracting accounts from device', () => {
  return expect(createSolanaLedgerKeyring().getAccounts(-2, 1))
    .resolves.toStrictEqual({
      payload: [
        {
          'address': bs58.encode(Buffer.from('address for 44\'/501\'/0\'/0')),
          'derivationPath': '44\'/501\'/0\'/0',
          'hardwareVendor': 'Ledger',
          'name': 'Ledger',
          'deviceId': 'device1',
          'coin': BraveWallet.CoinType.SOL
        },
        {
          'address': bs58.encode(Buffer.from('address for 44\'/501\'/0\'/1')),
          'derivationPath': '44\'/501\'/0\'/1',
          'hardwareVendor': 'Ledger',
          'name': 'Ledger',
          'deviceId': 'device1',
          'coin': BraveWallet.CoinType.SOL
        }],
      success: true
    }
    )
})

// TODO(nvonpentz) Implement tests for similar functionality if we support multiple derivation paths
// test('Extracting accounts from legacy device', () => {
//   return expect(createLedgerKeyring().getAccounts(-2, 1, LedgerDerivationPaths.Legacy))
//     .resolves.toStrictEqual({
//       payload: [
//         {
//           'address': 'address for m/44\'/60\'/0\'/0',
//           'derivationPath': 'm/44\'/60\'/0\'/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH
//         },
//         {
//           'address': 'address for m/44\'/60\'/0\'/1',
//           'derivationPath': 'm/44\'/60\'/0\'/1',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH
//         }],
//       success: true
//     }
//     )
// })

// test('Extracting accounts with deprecated derivation paths', () => {
//   return expect(createLedgerKeyring().getAccounts(-2, 1, LedgerDerivationPaths.Deprecated))
//     .resolves.toStrictEqual({
//       payload: [
//         {
//           'address': 'address for m/44\'/60\'/0\'/0',
//           'derivationPath': 'm/44\'/60\'/0\'/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH
//         },
//         {
//           'address': 'address for m/44\'/60\'/1\'/0',
//           'derivationPath': 'm/44\'/60\'/1\'/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH
//         }],
//       success: true
//     }
//     )
// })

test('Check ledger bridge type', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerKeyring()
  return expect(solanaLedgerHardwareKeyring.type()).toStrictEqual(BraveWallet.LEDGER_HARDWARE_VENDOR)
})

test('Check locks for device app only', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerKeyring()
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
  solanaLedgerHardwareKeyring.app = new MockApp()
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
})

test('Check locks for device app and device id', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerKeyring()
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
  solanaLedgerHardwareKeyring.app = new MockApp()
  solanaLedgerHardwareKeyring.deviceId = 'test'
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(true)
})

test('Extract accounts from locked device', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerKeyring()
  solanaLedgerHardwareKeyring.unlock = async function () {
    return { success: false, error: 'braveWalletUnlockError' }
  }
  return expect(solanaLedgerHardwareKeyring.getAccounts(-2, 1))
    .resolves.toStrictEqual({ error: 'braveWalletUnlockError', success: false })
})

// TODO(nvonpentz) Implement tests for similar functionality if relevant
// test('Extract accounts from unknown device', () => {
//   const solanaLedgerHardwareKeyring = unlockedLedgerKeyring()
//   return expect(solanaLedgerHardwareKeyring.getAccounts(-2, 1, '))
//     .rejects.toThrow()
// })

// test('Sign personal message successfully with padding v<27', () => {
//   const solanaLedgerHardwareKeyring = unlockedLedgerKeyring()
//   solanaLedgerHardwareKeyring.app.signature = { v: 0, r: 'b68983', s: 'r68983' }
//   return expect(solanaLedgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898300', success: true })
// })

// test('Sign personal message successfully with padding v>=27', () => {
//   const solanaLedgerHardwareKeyring = unlockedLedgerKeyring()
//   solanaLedgerHardwareKeyring.app.signature = { v: 28, r: 'b68983', s: 'r68983' }
//   return expect(solanaLedgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898301', success: true })
// })

// test('Sign personal message successfully without padding v>=27', () => {
//   const solanaLedgerHardwareKeyring = unlockedLedgerKeyring()
//   solanaLedgerHardwareKeyring.app.signature = { v: 44, r: 'b68983', s: 'r68983' }
//   return expect(solanaLedgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898311', success: true })
// })

// test('Sign personal message successfully without padding v<27', () => {
//   const solanaLedgerHardwareKeyring = unlockedLedgerKeyring()
//   solanaLedgerHardwareKeyring.app.signature = { v: 17, r: 'b68983', s: 'r68983' }
//   return expect(solanaLedgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898311', success: true })
// })

// test('Sign personal message failed', () => {
//   const solanaLedgerHardwareKeyring = createSolanaLedgerKeyring()
//   return expect(solanaLedgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toMatchObject({ success: false })
// })

// test('Sign typed message success', () => {
//   const app = new MockApp()
//   app.signature = { v: 28, r: 'b68983', s: 'r68983' }
//   const solanaLedgerHardwareKeyring = createSolanaLedgerKeyring(app)
//   return expect(solanaLedgerHardwareKeyring.signEip712Message(
//     'm/44\'/60\'/0\'/0/0', 'domainSeparatorHex', 'hashStructMessageHex'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898301', success: true })
// })

// test('Sign typed message locked', () => {
//   const solanaLedgerHardwareKeyring = new SolanaLedgerKeyring()
//   solanaLedgerHardwareKeyring.unlock = async () => {
//     return { success: false }
//   }
//   return expect(solanaLedgerHardwareKeyring.signEip712Message(
//     'm/44\'/60\'/0\'/0/0', 'domainSeparatorHex', 'hashStructMessageHex'))
//     .resolves.toStrictEqual({ success: false })
// })

// test('Sign typed message error', () => {
//   const app = new MockApp()
//   app.signEIP712HashedMessage = async (path: string,
//     domainSeparatorHex: string,
//     hashStructMessageHex: string) => {
//       throw Error('some error')
//   }
//   const solanaLedgerHardwareKeyring = createSolanaLedgerKeyring(app)
//   return expect(solanaLedgerHardwareKeyring.signEip712Message(
//     'm/44\'/60\'/0\'/0/0', 'domainSeparatorHex', 'hashStructMessageHex'))
//     .resolves.toStrictEqual({ success: false, error: 'some error', code: 'Error' })
// })
