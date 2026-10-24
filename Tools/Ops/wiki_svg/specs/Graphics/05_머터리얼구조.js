// 4-1. IRawMaterial과 GShaderLibrary — 생성 인자(도메인 · 프로퍼티) → PDX12Material → 템플릿 치환 → PDX12GraphicsShader 컴파일, PSO는 그릴 때(6번)
// 코드: Classes/Material.h(EMaterialDomain · HRawMaterialConstructArguments · IRawMaterial), Classes/ShaderLibrary.h/.cpp, DirectX12/DX12Material.h/.cpp(Compile), DirectX12/Classes/DX12Shader.h(PDX12GraphicsShader), DX12GraphicsCommand.cpp(PSO Finalize)
module.exports = {
  file: 'Graphics_05_머터리얼구조',
  alt: 'IRawMaterial과 GShaderLibrary — PJGGraphicsAPI::CreateRawMaterial이 도메인(Surface · Screen)과 프로퍼티 정의로 PDX12Material을 생성하고, Compile은 GShaderLibrary가 Content/Shaders에서 읽은 템플릿에 머터리얼 코드를 치환해 PDX12GraphicsShader로 컴파일하며, PSO는 그릴 때 그리기 명령이 바이트코드로 구성한다',
  type: 'class',
  w: 1000,
  h: 690,
  classes: [
    { id: 'ARG', x: 40, y: 30, w: 380, name: 'HRawMaterialConstructArguments', stereo: '머터리얼 생성 인자', color: 'orange', members: ['Domain : EMaterialDomain', '프로퍼티 정의 (이름 · 타입)'] },
    { id: 'DOM', x: 40, y: 170, w: 380, name: 'EMaterialDomain', stereo: '머터리얼 도메인', color: 'orange', members: ['Surface  // 메시 표면 → G버퍼', 'Screen  // 화면 전체 패스'] },
    { id: 'IRM', x: 560, y: 30, w: 400, name: 'IRawMaterial', stereo: '머터리얼 인터페이스', color: 'purple', members: ['+GetDomain()', '+SetFloat() · SetTexture() 등', '+Compile(shaderCode)'] },
    { id: 'MAT', x: 560, y: 200, w: 400, name: 'PDX12Material', stereo: 'DX12 머터리얼', color: 'blue', members: ['-_propertyDefinitionist  // 프로퍼티 정의', '-_materialConstantBuffer  // 상수 값', '-_materialTextures  // 텍스처 슬롯', '-_graphicsShader : PDX12GraphicsShader'] },
    { id: 'SHL', x: 40, y: 330, w: 380, name: 'GShaderLibrary', stereo: '셰이더 템플릿 · Core 전역 시스템으로 등록', color: 'green', members: ['+GetGraphicsShaderTemplateCode()', '+GetShaderCode(name)'] },
    { id: 'SHD', x: 560, y: 400, w: 400, name: 'PDX12GraphicsShader', stereo: '컴파일된 셰이더', color: 'blue', members: ['-_byteCodes : 셰이더 단계 → 바이트코드', '+Compile(args)  // FXC'] }
  ],
  nodes: [
    { id: 'CS', x: 40, y: 470, w: 380, h: 64, color: 'yellow', shape: 'cyl', title: 'Content/Shaders', lines: ['HLSL 템플릿 · 내장 2D 셰이더'] },
    { id: 'PSO', x: 560, y: 550, w: 400, h: 60, color: 'gray', title: 'PSO', lines: ['그릴 때 PDX12GraphicsCommand가 구성 · 전역 캐시'] },
    { id: 'KEY', x: 40, y: 580, w: 380, h: 80, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 생성 · 사용', '+ public · - private'] }
  ],
  edges: [
    { from: 'ARG', to: 'DOM', fs: 'b', ts: 't', label: 'Domain' },
    { points: [[420, 75], [490, 75], [490, 263], [560, 263]], label: 'CreateRawMaterial(args)', lx: 490, ly: 170 },
    { from: 'MAT', to: 'IRM', fs: 't', ts: 'b', end: 'triangle' },
    { points: [[560, 300], [510, 300], [510, 375], [420, 375]], label: '템플릿에 머터리얼 코드 치환', lx: 510, ly: 345 },
    { from: 'SHL', to: 'CS', fs: 'b', ts: 't', label: '실행 중 읽기' },
    { from: 'MAT', to: 'SHD', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: 'Compile' },
    { from: 'SHD', to: 'PSO', fs: 'b', ts: 't', dash: true, label: '바이트코드' }
  ]
};
