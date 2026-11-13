import { createHash } from 'node:crypto'
import { writeFile } from 'node:fs/promises'
import path from 'node:path'
import process from 'node:process'

const hash = (label) =>
  createHash('sha256')
    .update(`source-owned-tui-windows-profile:v1:${label}`)
    .digest('hex')

function parseArgs(argv) {
  const options = {}
  for (let index = 0; index < argv.length; index += 1) {
    const name = argv[index]
    if (!['--output', '--source-commit'].includes(name) || !argv[index + 1])
      throw new Error('usage')
    options[name.slice(2)] = argv[(index += 1)]
  }
  if (!options.output) throw new Error('--output required')
  return options
}

const options = parseArgs(process.argv.slice(2))
const sourceCommit = options['source-commit'] ?? process.env.GITHUB_SHA
if (!/^[a-f0-9]{40}$/u.test(sourceCommit ?? ''))
  throw new Error('source commit required')

const profile = {
  schemaVersion: 3,
  sourceCommit,
  providerProfileSHA256: hash('provider-profile'),
  regions: {
    endpointSecurity: {
      capacity: 1048576,
      originalSHA256: hash('endpointSecurity'),
    },
    processSecurity: {
      capacity: 1048576,
      originalSHA256: hash('processSecurity'),
    },
    tokenAccess: { capacity: 1048576, originalSHA256: hash('tokenAccess') },
  },
  arguments: {
    moduleUnitsIncludingNUL: 32768,
    originalSHA256: hash('arguments'),
  },
  argumentWriter: {
    stackReservationBytes: '1048576',
    turnCapacity: '64',
    originalSHA256: hash('argumentWriter'),
  },
  creatorImages: {
    moduleCapacity: '256',
    mappedNameUnitsIncludingNUL: 4096,
    fileBytes: '67108864',
    memoryBytes: '268435456',
    memoryRegionCapacity: '4096',
    originalSHA256: hash('creatorImages'),
  },
}

const destination = path.resolve(options.output)
await writeFile(destination, `${JSON.stringify(profile, null, 2)}\n`, {
  flag: 'wx',
})
process.stdout.write(`${destination}\n`)
